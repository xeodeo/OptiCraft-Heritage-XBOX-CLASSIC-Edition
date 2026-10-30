#include "ps3/spu/Ps3Spu.h"

#include <atomic>
#include <cstring>
#include <mutex>

#include <sys/spu.h>
#include <sys/systime.h>
#include <sys/thread.h>

#include "platform/Log.h"
#include "platform/PlatformCompat.h"
#include "platform/PlatformTuning.h"

#if PS3_SPU_TASKS
#include "ps3_spu_tasks_elf.h"
#endif

#ifndef PS3_SPU_THREADS
#define PS3_SPU_THREADS 4
#endif
#ifndef PS3_SPU_GROUP_PRIORITY
#define PS3_SPU_GROUP_PRIORITY 100
#endif

namespace
{
    constexpr int kMaxThreads = 6;

    // One 16-byte cell per SPU thread; the SPU writes it by DMA (SpuJob.h).
    struct alignas(16) SlotCell
    {
        volatile std::uint32_t state;
        std::uint32_t pad[3];
    };
    SlotCell s_slots[kMaxThreads] __attribute__((aligned(128)));

    std::once_flag s_initOnce;
    std::atomic<bool> s_available{false};
    std::atomic<bool> s_shutdownRequested{false};
    int s_threadCount = 0;
    sysSpuImage s_image;
    sys_spu_group_t s_group = 0;
    sys_spu_thread_t s_threads[kMaxThreads];

    std::atomic<long> s_statJobs{0};
    std::atomic<long> s_statFailed{0};
    std::atomic<long> s_statSpuTicks{0};
    std::atomic<long> s_statWaitMicros{0};

    // Polite busy-wait on the in-order PPU: the low SMT priority hint gives the
    // sibling hardware thread (the mesh worker) the core's issue slots.
    inline void spinLowPriority() { __asm__ volatile("or 1,1,1"); }
    inline void spinRestorePriority() { __asm__ volatile("or 2,2,2"); }

    bool claimSlotCas(int i)
    {
        std::uint32_t expected = PS3_SPU_SLOT_IDLE;
        return __atomic_compare_exchange_n(const_cast<std::uint32_t *>(&s_slots[i].state), &expected,
                                           PS3_SPU_SLOT_BUSY, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE);
    }

    std::uint32_t loadState(int i)
    {
        return __atomic_load_n(const_cast<std::uint32_t *>(&s_slots[i].state), __ATOMIC_ACQUIRE);
    }

    void destroyGroup(bool started)
    {
        if (started)
        {
            u32 cause = 0, status = 0;
            sysSpuThreadGroupTerminate(s_group, 0);
            sysSpuThreadGroupJoin(s_group, &cause, &status);
        }
        sysSpuThreadGroupDestroy(s_group);
        s_group = 0;
    }

    bool startGroup(int count)
    {
        static const char kGroupName[] = "opticraft spu";
        static const char kThreadName[] = "opticraft spu job";

        sysSpuThreadGroupAttribute groupAttr;
        sysSpuThreadGroupAttributeInitialize(groupAttr);
        sysSpuThreadGroupAttributeName(groupAttr, kGroupName);
        s32 r = sysSpuThreadGroupCreate(&s_group, static_cast<u32>(count), PS3_SPU_GROUP_PRIORITY, &groupAttr);
        if (r != 0)
        {
            MC_LOG_WARN("ps3.spu", "thread group of %d: create failed 0x%08x\n", count, (unsigned)r);
            return false;
        }

        for (int i = 0; i < count; ++i)
        {
            s_slots[i].state = PS3_SPU_SLOT_IDLE;
            sysSpuThreadAttribute attr;
            sysSpuThreadAttributeInitialize(attr);
            sysSpuThreadAttributeName(attr, kThreadName);
            sysSpuThreadArgument arg;
            sysSpuThreadArgumentInitialize(arg);
            arg.arg0 = static_cast<u64>(reinterpret_cast<std::uintptr_t>(&s_slots[i]));
            r = sysSpuThreadInitialize(&s_threads[i], s_group, static_cast<u32>(i), &s_image, &attr, &arg);
            if (r == 0)
                r = sysSpuThreadSetConfiguration(s_threads[i], SPU_SIGNAL1_OVERWRITE | SPU_SIGNAL2_OVERWRITE);
            if (r != 0)
            {
                MC_LOG_WARN("ps3.spu", "SPU thread %d of %d: init failed 0x%08x\n", i, count, (unsigned)r);
                destroyGroup(false);
                return false;
            }
        }

        r = sysSpuThreadGroupStart(s_group);
        if (r != 0)
        {
            MC_LOG_WARN("ps3.spu", "thread group of %d: start failed 0x%08x\n", count, (unsigned)r);
            destroyGroup(false);
            return false;
        }
        return true;
    }

    void init()
    {
#if PS3_SPU_TASKS
        const std::uint64_t t0 = PlatformCompat::getMonotonicMicros();
        // Six SPUs is what a game gets; an error here only means another
        // library already did it.
        const s32 initResult = sysSpuInitialize(6, 0);

        // The ELF is linked 128-byte aligned (cmake/ps3_embed_binary.cmake).
        const s32 importResult = sysSpuImageImport(&s_image, ps3_spu_tasks_elf, SPU_IMAGE_PROTECT);
        if (importResult != 0)
        {
            MC_LOG_WARN("ps3.spu", "SPU image import failed 0x%08x (init 0x%08x); everything stays on the PPU\n",
                        (unsigned)importResult, (unsigned)initResult);
            return;
        }

        int count = PS3_SPU_THREADS < kMaxThreads ? PS3_SPU_THREADS : kMaxThreads;
        for (; count > 0; --count)
            if (startGroup(count))
                break;
        if (count == 0)
        {
            sysSpuImageClose(&s_image);
            MC_LOG_WARN("ps3.spu", "no SPU thread group could start; everything stays on the PPU\n");
            return;
        }

        s_threadCount = count;
        s_available.store(true, std::memory_order_release);
        MC_LOG_INFO("ps3.spu", "SPU runtime started: %d threads, priority %d, image %u bytes, %ld us (init 0x%08x)\n",
                    count, PS3_SPU_GROUP_PRIORITY, (unsigned)ps3_spu_tasks_elf_size,
                    (long)(PlatformCompat::getMonotonicMicros() - t0), (unsigned)initResult);
#else
        MC_LOG_INFO("ps3.spu", "built without SPU tasks (PS3_SPU_TASKS=0); everything stays on the PPU\n");
#endif
    }
}

bool ps3SpuAvailable()
{
    // Never (re)start the group on the way out.
    if (s_shutdownRequested.load(std::memory_order_acquire))
        return false;
    std::call_once(s_initOnce, init);
    return s_available.load(std::memory_order_acquire);
}

int ps3SpuThreadCount()
{
    return ps3SpuAvailable() ? s_threadCount : 0;
}

bool ps3SpuSubmit(Ps3SpuJob &job, std::uint32_t type, const void *input)
{
    job.slot = -1;
    if (!ps3SpuAvailable())
        return false;

    const std::uintptr_t headerEA = reinterpret_cast<std::uintptr_t>(&job.header);
    const std::uintptr_t inputEA = reinterpret_cast<std::uintptr_t>(input);
    // The signal carries a 32-bit effective address; everything we hand the
    // SPU is 16-byte aligned for its DMA.
    if ((headerEA >> 32) != 0 || (headerEA & 127) != 0 || (inputEA & 15) != 0)
        return false;

    std::memset(&job.header, 0, sizeof(job.header));
    job.header.type = type;
    job.header.status = PS3_SPU_STATUS_PENDING;
    job.header.eaData = static_cast<unsigned long long>(inputEA);

    // A free SPU thread. All of them busy means jobs in flight that someone
    // will wait for; their SPU frees the slot by itself when it finishes.
    int slot = -1;
    bool spun = false;
    for (;;)
    {
        if (!s_available.load(std::memory_order_acquire))
            break;
        for (int i = 0; i < s_threadCount; ++i)
        {
            if (claimSlotCas(i))
            {
                slot = i;
                break;
            }
        }
        if (slot >= 0)
            break;
        spun = true;
        spinLowPriority();
    }
    if (spun)
        spinRestorePriority();
    if (slot < 0)
        return false;

    job.slot = slot;
    job.submitMicros = PlatformCompat::getMonotonicMicros();
    // The header and the input are in memory before the SPU can read them.
    __atomic_thread_fence(__ATOMIC_SEQ_CST);
    const s32 r = sysSpuThreadWriteSignal(s_threads[slot], 0, static_cast<u32>(headerEA));
    if (r != 0)
    {
        __atomic_store_n(const_cast<std::uint32_t *>(&s_slots[slot].state), PS3_SPU_SLOT_IDLE, __ATOMIC_RELEASE);
        job.slot = -1;
        MC_LOG_WARN("ps3.spu", "signal to SPU thread %d failed 0x%08x\n", slot, (unsigned)r);
        return false;
    }
    return true;
}

bool ps3SpuWait(Ps3SpuJob &job)
{
    if (job.slot < 0)
        return false;

    const std::uint64_t t0 = PlatformCompat::getMonotonicMicros();
    volatile std::uint32_t *status = &job.header.status;
    std::uint32_t value = __atomic_load_n(const_cast<std::uint32_t *>(status), __ATOMIC_ACQUIRE);
    if (value == PS3_SPU_STATUS_PENDING)
    {
        int spins = 0;
        while ((value = __atomic_load_n(const_cast<std::uint32_t *>(status), __ATOMIC_ACQUIRE)) ==
               PS3_SPU_STATUS_PENDING)
        {
            spinLowPriority();
            // Past a couple of milliseconds, give the core away instead.
            if (++spins > 4096)
            {
                spinRestorePriority();
                sysUsleep(20);
            }
        }
        spinRestorePriority();
    }
    const std::uint64_t t1 = PlatformCompat::getMonotonicMicros();

    job.slot = -1;
    s_statJobs.fetch_add(1, std::memory_order_relaxed);
    s_statWaitMicros.fetch_add(static_cast<long>(t1 - t0), std::memory_order_relaxed);
    s_statSpuTicks.fetch_add(static_cast<long>(job.header.spuTicks), std::memory_order_relaxed);
    if (value != PS3_SPU_STATUS_DONE)
    {
        s_statFailed.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    return true;
}

Ps3SpuStats ps3SpuTakeStats()
{
    Ps3SpuStats stats;
    stats.jobs = s_statJobs.exchange(0, std::memory_order_relaxed);
    stats.failed = s_statFailed.exchange(0, std::memory_order_relaxed);
    stats.waitMicros = s_statWaitMicros.exchange(0, std::memory_order_relaxed);
    // SPU decrementer = timebase, 79.8 MHz on every PS3.
    stats.spuMicros = static_cast<long>(static_cast<long long>(s_statSpuTicks.exchange(0, std::memory_order_relaxed)) *
                                        10 / 798);
    return stats;
}

void ps3SpuShutdown()
{
    s_shutdownRequested.store(true, std::memory_order_release);
    if (!s_available.exchange(false, std::memory_order_acq_rel))
        return;

    const std::uint64_t t0 = PlatformCompat::getMonotonicMicros();
    // Jobs still running (none expected: generation runs on the main thread,
    // which is the one quitting) get up to 100 ms.
    for (int i = 0; i < s_threadCount; ++i)
        for (int waited = 0; waited < 100 && loadState(i) == PS3_SPU_SLOT_BUSY; ++waited)
            sysUsleep(1000);

    for (int i = 0; i < s_threadCount; ++i)
        if (loadState(i) == PS3_SPU_SLOT_IDLE)
            sysSpuThreadWriteSignal(s_threads[i], 0, PS3_SPU_SIGNAL_QUIT);

    // Each thread reports "exited" in its slot just before sys_spu_thread_exit.
    bool allExited = false;
    for (int waited = 0; waited < 200 && !allExited; ++waited)
    {
        allExited = true;
        for (int i = 0; i < s_threadCount; ++i)
            if (loadState(i) != PS3_SPU_SLOT_EXITED)
                allExited = false;
        if (!allExited)
            sysUsleep(500);
    }

    u32 cause = 0, status = 0;
    if (!allExited)
        sysSpuThreadGroupTerminate(s_group, 0);
    sysSpuThreadGroupJoin(s_group, &cause, &status);
    sysSpuThreadGroupDestroy(s_group);
    sysSpuImageClose(&s_image);
    MC_LOG_INFO("ps3.spu", "SPU runtime stopped: %d threads %s in %ld us\n", s_threadCount,
                allExited ? "exited" : "terminated", (long)(PlatformCompat::getMonotonicMicros() - t0));
}
