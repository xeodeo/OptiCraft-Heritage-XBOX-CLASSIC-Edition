#include "ps3/render/Ps3MeshWorker.h"

#if PLATFORM_THREADED_TERRAIN_MESH

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <vector>

#include <sys/sem.h>
#include <sys/systime.h>
#include <sys/thread.h>

#include "net/minecraft/src/Block.h"
#include "net/minecraft/src/BlockAnvil.h"
#include "net/minecraft/src/RenderBlocks.h"
#include "net/minecraft/src/Tessellator.h"
#include "pc/render/PcLegacyBlockRenderInfo.h"
#include "pc/render/PcLegacyCubeMaterialInfo.h"
#include "pc/render/PcLegacyTerrainStaging.h"
#include "platform/Log.h"
#include "platform/PlatformCompat.h"
#include "platform/PlatformMeshThread.h"
#include "platform/PlatformTuning.h"
#if PLATFORM_SPU_CHUNK_MESH
#include "ps3/spu/Ps3SpuChunkMesh.h"
#endif

#ifndef PS3_MESH_WORKER_JOBS
#define PS3_MESH_WORKER_JOBS 3
#endif
#ifndef PS3_MESH_WORKER_PRIORITY
#define PS3_MESH_WORKER_PRIORITY 1100
#endif
#ifndef PS3_MESH_WORKER_STACK_BYTES
#define PS3_MESH_WORKER_STACK_BYTES (512 * 1024)
#endif

std::uintptr_t g_platformMeshWorkerStackLow = 0;
std::uintptr_t g_platformMeshWorkerStackSpan = 0;

namespace
{
    // Free and Filling belong to the main thread, Busy to the worker, Done to
    // the main thread again. Queued is the handover: the worker takes a job
    // with Queued -> Busy, the main thread takes one back (release before the
    // worker got to it) with Queued -> Free. Both are compare-exchanges, so
    // exactly one side wins.
    enum JobState : int
    {
        kJobFree,
        kJobFilling,
        kJobQueued,
        kJobBusy,
        kJobDone,
    };

    struct MeshJob
    {
        PcLegacyTerrainStaging staging;
        std::atomic<int> state{kJobFree};
        // Set by the main thread when the owner lets go while the worker has the
        // job; the worker stops at its next check and the result is dropped.
        std::atomic<bool> cancelled{false};
        // Main thread only. Null for a job whose renderer let go of it.
        WorldRenderer *owner = nullptr;
        // Submission order; the worker takes the oldest queued job first, and the
        // scheduler submits nearest-first.
        std::uint32_t sequence = 0;
        // Written by the worker before it publishes Done.
        std::uint32_t workerMicros = 0;
        int outOfRangeReads = 0;
        // Worker only: precomputed and (if it could) handed to an SPU ahead of
        // meshing, so several sections' SPU jobs run at once.
        bool prepared = false;
        bool spuBegun = false;
    };

    MeshJob s_jobs[PS3_MESH_WORKER_JOBS];
#if PLATFORM_SPU_CHUNK_MESH
    static_assert(PS3_MESH_WORKER_JOBS <= kPs3SpuMeshTickets, "one SPU mesh ticket per worker job");
#endif
    std::uint32_t s_nextSequence = 1;

    sys_ppu_thread_t s_thread = 0;
    sys_sem_t s_wake = 0;
    bool s_startAttempted = false;
    std::atomic<bool> s_running{false};
    std::atomic<bool> s_stop{false};
    std::atomic<bool> s_exited{false};

    // Counters for the periodic "ps3.mesh" line. The worker's are atomics, the
    // rest are main-thread only.
    std::atomic<long> s_statMeshed{0};
    std::atomic<long> s_statWorkerMicros{0};
    std::atomic<long> s_statOutOfRange{0};
    std::atomic<long> s_statFailed{0};
    long s_statSubmitted = 0;
    long s_statDropped = 0;
    long s_statSnapshots = 0;
    long s_statSnapshotMicros = 0;
    long s_statPublishes = 0;
    long s_statPublishMicros = 0;
    std::uint64_t s_statLastLogUs = 0;

    void resetJob(MeshJob &job)
    {
        PcLegacyTerrainStaging &staging = job.staging;
        staging.sectionCache.reset();
        staging.sourceCache.reset();
        for (int pass = 0; pass < 2; ++pass)
        {
            staging.compactPassMeshes[pass].clear();
            staging.passMeshes[pass].clear();
            // Keep the vectors' capacity: the next section reuses it instead of
            // growing a fresh one on a heap the game already fragments.
            staging.extraTextureMeshes[pass].clear();
        }
        staging.tileEntityRenderers.clear();
        staging.staticTileEntityCandidates.clear();
        staging.owner = nullptr;
        staging.skyLit = false;
        staging.buildFailed = false;
        job.owner = nullptr;
        job.cancelled.store(false, std::memory_order_relaxed);
        job.workerMicros = 0;
        job.outOfRangeReads = 0;
        job.state.store(kJobFree, std::memory_order_release);
    }

    MeshJob *jobOf(const WorldRenderer *owner)
    {
        if (owner == nullptr)
            return nullptr;
        for (MeshJob &job : s_jobs)
        {
            if (job.owner == owner)
                return &job;
        }
        return nullptr;
    }

    // -----------------------------------------------------------------------
    // Worker thread
    // -----------------------------------------------------------------------

    MeshJob *takeNextJob()
    {
        for (;;)
        {
            MeshJob *oldest = nullptr;
            for (MeshJob &job : s_jobs)
            {
                if (job.state.load(std::memory_order_acquire) != kJobQueued)
                    continue;
                if (oldest == nullptr || static_cast<std::int32_t>(job.sequence - oldest->sequence) < 0)
                    oldest = &job;
            }
            if (oldest == nullptr)
                return nullptr;
            int expected = kJobQueued;
            if (oldest->state.compare_exchange_strong(expected, kJobBusy, std::memory_order_acq_rel))
                return oldest;
            // The main thread took it back meanwhile; look again.
        }
    }

    // The section loop of WorldRenderer::pcLegacyBuildRendererStep, whole
    // section at once and without the tile entity scan (the main thread did it
    // while taking the snapshot). Reads only the detached section cache.
    // Face masks and work lists, then the opaque pass's SPU job. Done for a
    // whole batch of taken jobs before any is meshed: each SPU job then runs
    // while this thread renders the earlier sections' leftovers.
    void prepareJob(MeshJob &job)
    {
        job.prepared = true;
        job.spuBegun = false;
        PcLegacyTerrainStaging &staging = job.staging;
        if (!staging.sectionCache.has_value())
            return;
        PcLegacySectionCache &sectionCache = *staging.sectionCache;
        sectionCache.precomputeDeferredSectionData();
#if PLATFORM_SPU_CHUNK_MESH
        if (!job.cancelled.load(std::memory_order_relaxed) && !s_stop.load(std::memory_order_relaxed))
            job.spuBegun = ps3SpuMeshBegin(static_cast<int>(&job - s_jobs), sectionCache);
#endif
    }

    bool meshJob(MeshJob &job, Tessellator &tessellator)
    {
        PcLegacyTerrainStaging &staging = job.staging;
        if (!staging.sectionCache.has_value())
            return false;

        if (!job.prepared)
            prepareJob(job);
        PcLegacySectionCache &sectionCache = *staging.sectionCache;

        const int_t x0 = staging.originX;
        const int_t y0 = staging.originY;
        const int_t z0 = staging.originZ;
        bool ok = true;
        for (int_t pass = 0; pass < 2 && ok; ++pass)
        {
            const int_t workCount = sectionCache.getRenderWorkCount(pass);
            if (workCount <= 0)
                continue;

            sectionCache.beginStep();
#if PLATFORM_SPU_CHUNK_MESH
            // Opaque pass: the SPU meshes the plain cubes and grass while this
            // thread renders everything else.
            const int spuSlot = static_cast<int>(&job - s_jobs);
            const bool spu = pass == 0 && job.spuBegun;
            int ppuBlocks = 0;
#endif
            RenderBlocks renderblocks(&sectionCache);
            renderblocks.setMeshTessellator(&tessellator);
            if (PLATFORM_COMPACT_TERRAIN_VERTICES)
                renderblocks.setPcLegacyCompactTerrainMesh(&staging.compactPassMeshes[pass], x0, y0, z0);
            tessellator.startDrawingQuads();
            tessellator.setTranslationD(-static_cast<double>(x0), -static_cast<double>(y0), -static_cast<double>(z0));

            bool aborted = false;
            for (int_t work = 0; work < workCount; ++work)
            {
                if ((work & 63) == 0 &&
                    (job.cancelled.load(std::memory_order_relaxed) || s_stop.load(std::memory_order_relaxed)))
                {
                    aborted = true;
                    break;
                }

                const int_t cursor = static_cast<int_t>(sectionCache.getRenderWorkEntry(pass, work));
                const int_t lx = cursor & 15;
                const int_t lz = (cursor >> 4) & 15;
                const int_t ly = (cursor >> 8) & 15;
                const int_t id = sectionCache.getBlockIdLocal(lx, ly, lz);
                if (id <= 0 || id >= Block::BLOCK_REGISTRY_SIZE)
                    continue;
                Block *block = Block::blocksList[id];
                if (block == nullptr)
                    continue;

                const PcLegacyBlockRenderInfo &blockInfo = pcLegacyGetBlockRenderInfo(id);
                if (blockInfo.renderPass != pass)
                    continue;
#if PLATFORM_SPU_CHUNK_MESH
                if (spu && ps3SpuMeshHandles(spuSlot, id))
                    continue;
                ++ppuBlocks;
#endif

                if (blockInfo.simpleOpaqueCube)
                {
                    const std::uint8_t faceMask = sectionCache.getExposedFaceMaskLocal(lx, ly, lz);
                    if (faceMask != 0)
                        renderblocks.renderSimpleOpaqueCubeLegacy(block, x0 + lx, y0 + ly, z0 + lz, faceMask);
                }
                else
                {
                    renderblocks.renderBlockByRenderType(block, x0 + lx, y0 + ly, z0 + lz);
                }
            }

#if PLATFORM_SPU_CHUNK_MESH
            // Always wait for the SPU (its job reads this snapshot), then add
            // its quads to the batch; a failed or unverified SPU result is
            // rendered here on the PPU instead.
            if (spu)
                ps3SpuMeshEnd(spuSlot, sectionCache, tessellator, x0, y0, z0);
            if (pass == 0)
                ps3SpuMeshNotePpuBlocks(ppuBlocks);
#endif
            // Always consume the batch, aborted or not: a texture group left
            // drawing would carry its vertices into the next section.
            const bool groupsOk = tessellator.captureTextureGroups(staging.extraTextureMeshes[pass], true);
            const bool hadBaseVertices = tessellator.hasPendingVertices();
            const bool baseOk = tessellator.capture(staging.passMeshes[pass], true);
            tessellator.setTranslationD(0.0, 0.0, 0.0);
            if (aborted || !groupsOk || (hadBaseVertices && !baseOk))
                ok = false;
        }

        staging.skyLit = sectionCache.snapshotSkyLit();
        job.outOfRangeReads = sectionCache.snapshotOutOfRangeReads();
        return ok;
    }

    void workerEntry(void *)
    {
        // Claim this thread's stack for platformOnMeshWorker(): everything the
        // worker calls runs below this frame. The claimed span stays well inside
        // the stack lv2 allocated, so no other thread's stack pointer can match.
        char marker = 0;
        const std::uintptr_t top = reinterpret_cast<std::uintptr_t>(&marker) + 64;
        const std::uintptr_t span = static_cast<std::uintptr_t>(PS3_MESH_WORKER_STACK_BYTES) - 32 * 1024;
        g_platformMeshWorkerStackLow = top - span;
        std::atomic_thread_fence(std::memory_order_seq_cst);
        g_platformMeshWorkerStackSpan = span;
        std::atomic_thread_fence(std::memory_order_seq_cst);

        Tessellator *tessellator = new Tessellator(PLATFORM_TESSELLATOR_BUFFER_INTS);
        while (!s_stop.load(std::memory_order_acquire))
        {
            // Take every queued job (oldest first) and prepare them all, so
            // their SPU jobs start together; then mesh them in order.
            MeshJob *batch[PS3_MESH_WORKER_JOBS];
            int batchCount = 0;
            while (batchCount < PS3_MESH_WORKER_JOBS)
            {
                MeshJob *taken = takeNextJob();
                if (taken == nullptr)
                    break;
                taken->prepared = false;
                taken->spuBegun = false;
                batch[batchCount++] = taken;
            }
            if (batchCount == 0)
            {
                sysSemWait(s_wake, 100 * 1000);
                continue;
            }
            if (batchCount > 1)
                for (int b = 0; b < batchCount; ++b)
                    prepareJob(*batch[b]);

            for (int b = 0; b < batchCount; ++b)
            {
                MeshJob *job = batch[b];

                const std::uint64_t startUs = PlatformCompat::getMonotonicMicros();
                bool ok = false;
                try
                {
                    ok = meshJob(*job, *tessellator);
                }
                catch (...)
                {
#if PLATFORM_SPU_CHUNK_MESH
                    ps3SpuMeshCancel(static_cast<int>(job - s_jobs));
#endif
                    // A throw inside a block renderer leaves the tessellator mid
                    // batch; start the next section from a fresh one.
                    delete tessellator;
                    tessellator = new Tessellator(PLATFORM_TESSELLATOR_BUFFER_INTS);
                    ok = false;
                }
                const std::uint64_t endUs = PlatformCompat::getMonotonicMicros();
                job->workerMicros = endUs > startUs ? static_cast<std::uint32_t>(endUs - startUs) : 0u;
                job->staging.buildFailed = !ok;
                s_statWorkerMicros.fetch_add(static_cast<long>(job->workerMicros), std::memory_order_relaxed);
                s_statOutOfRange.fetch_add(job->outOfRangeReads, std::memory_order_relaxed);
                if (ok)
                    s_statMeshed.fetch_add(1, std::memory_order_relaxed);
                else
                    s_statFailed.fetch_add(1, std::memory_order_relaxed);
                job->state.store(kJobDone, std::memory_order_release);
            }
        }
        delete tessellator;

        g_platformMeshWorkerStackSpan = 0;
        s_exited.store(true, std::memory_order_release);
        sysThreadExit(0);
    }

    // -----------------------------------------------------------------------
    // Main thread
    // -----------------------------------------------------------------------

    void startWorker()
    {
        s_startAttempted = true;

        // Lazily built tables the worker reads must exist before it runs: the
        // block render info is filled on first use with no lock.
        (void)pcLegacyGetBlockRenderInfo(0);
        (void)pcLegacyGetVanillaFacePackedColor(0);

        // The worker's copies of the per-thread block state start from the main
        // thread's (the constructors only wrote the main copies).
        for (int_t id = 0; id < Block::BLOCK_REGISTRY_SIZE; ++id)
        {
            Block *block = Block::blocksList[id];
            if (block == nullptr)
                continue;
            block->minX.syncWorkerFromMain();
            block->minY.syncWorkerFromMain();
            block->minZ.syncWorkerFromMain();
            block->maxX.syncWorkerFromMain();
            block->maxY.syncWorkerFromMain();
            block->maxZ.syncWorkerFromMain();
            if (BlockAnvil *anvil = dynamic_cast<BlockAnvil *>(block))
                anvil->field_82521_b.syncWorkerFromMain();
        }

        sys_sem_attr_t attr = {};
        attr.attr_protocol = SYS_SEM_ATTR_PROTOCOL;
        attr.attr_pshared = SYS_SEM_ATTR_PSHARED;
        attr.name[0] = 'm'; attr.name[1] = 'c'; attr.name[2] = 'm'; attr.name[3] = 'e';
        attr.name[4] = 's'; attr.name[5] = 'h';
        if (sysSemCreate(&s_wake, &attr, 0, 1024) != 0)
        {
            MC_LOG_WARN("ps3.mesh", "worker semaphore failed; meshing stays on the main thread\n");
            return;
        }
        if (sysThreadCreate(&s_thread, workerEntry, nullptr, PS3_MESH_WORKER_PRIORITY,
                            PS3_MESH_WORKER_STACK_BYTES, THREAD_JOINABLE, const_cast<char *>("mesh worker")) != 0)
        {
            sysSemDestroy(s_wake);
            MC_LOG_WARN("ps3.mesh", "worker thread failed; meshing stays on the main thread\n");
            return;
        }
        s_running.store(true, std::memory_order_release);
        MC_LOG_INFO("ps3.mesh", "mesh worker started (%d jobs, priority %d, stack %d KB)\n",
                    PS3_MESH_WORKER_JOBS, PS3_MESH_WORKER_PRIORITY, PS3_MESH_WORKER_STACK_BYTES / 1024);
    }

    void logCounters()
    {
        const std::uint64_t nowUs = PlatformCompat::getMonotonicMicros();
        if (s_statLastLogUs == 0)
        {
            s_statLastLogUs = nowUs;
            return;
        }
        if (nowUs - s_statLastLogUs < 5u * 1000u * 1000u)
            return;
        s_statLastLogUs = nowUs;

        const long meshed = s_statMeshed.exchange(0, std::memory_order_relaxed);
        const long workerUs = s_statWorkerMicros.exchange(0, std::memory_order_relaxed);
        const long outOfRange = s_statOutOfRange.exchange(0, std::memory_order_relaxed);
        const long failed = s_statFailed.exchange(0, std::memory_order_relaxed);
        if (meshed == 0 && failed == 0 && s_statSubmitted == 0 && s_statPublishes == 0)
            return;
        const long jobs = meshed + failed;
        MC_LOG_INFO("ps3.mesh",
            "worker: queued=%ld meshed=%ld failed=%ld dropped=%ld workerMs=%ld (%ld us/section) "
            "snapshot=%ld us/section publish=%ld us/section outOfSnapshot=%ld\n",
            s_statSubmitted, meshed, failed, s_statDropped, workerUs / 1000,
            jobs > 0 ? workerUs / jobs : 0L,
            s_statSnapshots > 0 ? s_statSnapshotMicros / s_statSnapshots : 0L,
            s_statPublishes > 0 ? s_statPublishMicros / s_statPublishes : 0L,
            outOfRange);
        s_statSubmitted = 0;
        s_statDropped = 0;
        s_statSnapshots = 0;
        s_statSnapshotMicros = 0;
        s_statPublishes = 0;
        s_statPublishMicros = 0;
    }
}

bool ps3MeshWorkerRunning()
{
    return s_running.load(std::memory_order_relaxed);
}

void ps3MeshWorkerPump()
{
    if (!s_startAttempted)
        startWorker();
    if (!ps3MeshWorkerRunning())
        return;

    for (MeshJob &job : s_jobs)
    {
        if (job.owner == nullptr && job.state.load(std::memory_order_acquire) == kJobDone)
            resetJob(job);
    }
    logCounters();
#if PLATFORM_SPU_CHUNK_MESH
    ps3SpuMeshLogCounters();
#endif
}

PcLegacyTerrainStaging *ps3MeshWorkerAcquire(WorldRenderer *owner)
{
    if (!ps3MeshWorkerRunning() || owner == nullptr || jobOf(owner) != nullptr)
        return nullptr;
    for (MeshJob &job : s_jobs)
    {
        if (job.owner == nullptr && job.state.load(std::memory_order_acquire) == kJobFree)
        {
            job.state.store(kJobFilling, std::memory_order_relaxed);
            job.owner = owner;
            job.staging.owner = owner;
            return &job.staging;
        }
    }
    return nullptr;
}

void ps3MeshWorkerSubmit(PcLegacyTerrainStaging *staging)
{
    for (MeshJob &job : s_jobs)
    {
        if (&job.staging != staging)
            continue;
        if (job.state.load(std::memory_order_relaxed) != kJobFilling)
            return;
        job.sequence = s_nextSequence++;
        job.cancelled.store(false, std::memory_order_relaxed);
        job.state.store(kJobQueued, std::memory_order_release);
        sysSemPost(s_wake, 1);
        ++s_statSubmitted;
        return;
    }
}

bool ps3MeshWorkerOwnsJob(const WorldRenderer *owner)
{
    return jobOf(owner) != nullptr;
}

PcLegacyTerrainStaging *ps3MeshWorkerFinished(WorldRenderer *owner)
{
    MeshJob *job = jobOf(owner);
    if (job == nullptr || job->state.load(std::memory_order_acquire) != kJobDone)
        return nullptr;
    return &job->staging;
}

void ps3MeshWorkerRelease(const WorldRenderer *owner)
{
    MeshJob *job = jobOf(owner);
    if (job == nullptr)
        return;

    int state = job->state.load(std::memory_order_acquire);
    if (state == kJobQueued)
    {
        int expected = kJobQueued;
        if (job->state.compare_exchange_strong(expected, kJobFilling, std::memory_order_acq_rel))
            state = kJobFilling;
        else
            state = expected;
    }

    if (state == kJobFree || state == kJobFilling || state == kJobDone)
    {
        resetJob(*job);
        return;
    }

    // Busy: the worker owns the data until it publishes Done. Detach the job
    // from its renderer now and let the pump free it then.
    job->cancelled.store(true, std::memory_order_relaxed);
    job->owner = nullptr;
    ++s_statDropped;
}

void ps3MeshWorkerForgetTileEntity(const void *tileEntity)
{
    // The worker never touches tileEntityRenderers, whatever the job's state.
    for (MeshJob &job : s_jobs)
    {
        std::vector<TileEntity *> &list = job.staging.tileEntityRenderers;
        list.erase(std::remove(list.begin(), list.end(), tileEntity), list.end());
    }
}

void ps3MeshWorkerNoteSnapshot(std::uint64_t micros)
{
    ++s_statSnapshots;
    s_statSnapshotMicros += static_cast<long>(micros);
}

void ps3MeshWorkerNotePublish(std::uint64_t micros)
{
    ++s_statPublishes;
    s_statPublishMicros += static_cast<long>(micros);
}

void ps3MeshWorkerShutdown()
{
    if (!ps3MeshWorkerRunning())
        return;

    s_stop.store(true, std::memory_order_release);
    sysSemPost(s_wake, 1);

    // The worker checks the stop flag every 64 blocks, so it is out within a
    // millisecond or two. Never hold the exit on it, though: the XMB reboots
    // the console when a quit takes too long, and lv2 ends the thread with the
    // process anyway.
    for (int waited = 0; waited < 500 && !s_exited.load(std::memory_order_acquire); ++waited)
        sysUsleep(1000);
    if (s_exited.load(std::memory_order_acquire))
    {
        u64 result = 0;
        sysThreadJoin(s_thread, &result);
        sysSemDestroy(s_wake);
        MC_LOG_INFO("ps3.mesh", "mesh worker stopped\n");
    }
    else
    {
        MC_LOG_WARN("ps3.mesh", "mesh worker did not stop in time; leaving it to the process exit\n");
    }

    // From here on the synchronous builder runs; renderers that still believe
    // a job is in flight find none and start over.
    s_running.store(false, std::memory_order_release);
    for (MeshJob &job : s_jobs)
    {
        job.owner = nullptr;
        if (s_exited.load(std::memory_order_acquire))
            resetJob(job);
    }
}

#endif
