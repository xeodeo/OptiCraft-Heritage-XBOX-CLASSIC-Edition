#include "pc/render/PcLegacyMeshScheduler.h"

#if PLATFORM_INCREMENTAL_TERRAIN_BUILD

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "java/Arithmetic.h"
#include "net/minecraft/src/EntityLiving.h"
#include "net/minecraft/src/World.h"
#include "net/minecraft/src/WorldRenderer.h"
#include "pc/tuning/PcLegacyTuning.h"
#include "platform/PlatformCompat.h"
#include "platform/PlatformTuning.h"

namespace
{
    struct PcLegacyMeshCandidate
    {
        WorldRenderer *renderer;
        float distance;
        bool active;
        bool inFrustum;
    };

    static std::vector<PcLegacyMeshCandidate> s_candidates;
    // Scheduler counters since the last take (Xbox profile report).
    static long s_statPending = 0;
    static long s_statSteps = 0;
    static long s_statPublished = 0;
    static long s_statStepUs = 0;
    static long s_statCalls = 0;

    class PcLegacyFrameMeshBudget
    {
    public:
        explicit PcLegacyFrameMeshBudget(int_t requestedUpdateLimit)
            : startUs(PlatformCompat::getMonotonicMicros()),
              spentStepUs(0),
              steps(0),
              maxSteps(std::max(1, std::min(
                  PC_LEGACY_SCHEDULER_MAX_STEPS_PER_FRAME,
                  std::max(1, requestedUpdateLimit) * PC_LEGACY_SCHEDULER_STEPS_PER_UPDATE_UNIT)))
        {
        }

        bool exhausted() const
        {
            if (steps >= maxSteps)
                return true;

            if (PLATFORM_CHUNK_BUILD_BUDGET_MS <= 0)
                return false;

            const std::uint64_t budgetUs =
                static_cast<std::uint64_t>(PLATFORM_CHUNK_BUILD_BUDGET_MS) * 1000u;
            const std::uint64_t nowUs = PlatformCompat::getMonotonicMicros();
            std::uint64_t elapsedUs = spentStepUs;
            if (startUs != 0 && nowUs > startUs)
                elapsedUs = std::max(elapsedUs, nowUs - startUs);

            if (elapsedUs >= budgetUs)
                return true;

            return steps > 0 &&
                elapsedUs + static_cast<std::uint64_t>(PLATFORM_CHUNK_BUILD_STEP_US) > budgetUs;
        }

        void run(WorldRenderer *renderer)
        {
            const std::uint64_t stepStartUs = PlatformCompat::getMonotonicMicros();
            const int_t publishedBefore = WorldRenderer::chunksUpdated;
            renderer->updateRenderer();
            const std::uint64_t stepEndUs = PlatformCompat::getMonotonicMicros();
            s_statPublished += static_cast<long>(WorldRenderer::chunksUpdated - publishedBefore);
            ++s_statSteps;
            if (stepEndUs > stepStartUs)
                s_statStepUs += static_cast<long>(stepEndUs - stepStartUs);

            ++steps;
            if (stepEndUs > stepStartUs)
                spentStepUs += stepEndUs - stepStartUs;
        }

    private:
        std::uint64_t startUs;
        std::uint64_t spentStepUs;
        int_t steps;
        int_t maxSteps;
    };
}

// Scheduler counters since the last take (Xbox profile report).

void pcLegacyTakeMeshSchedulerStats(long *calls, long *pending, long *steps, long *published, long *stepUs)
{
    *calls = s_statCalls;
    *pending = s_statPending;
    *steps = s_statSteps;
    *published = s_statPublished;
    *stepUs = s_statStepUs;
    s_statCalls = s_statPending = s_statSteps = s_statPublished = s_statStepUs = 0;
}

void pcLegacyRunMeshScheduler(
    const std::vector<WorldRenderer *> &pending,
    EntityLiving *viewer,
    bool frustumOnly,
    int_t requestedUpdateLimit)
{
    const std::size_t fixedRendererCapacity = static_cast<std::size_t>(
        PC_LEGACY_VISIBLE_CHUNK_DIAMETER *
        PC_LEGACY_VISIBLE_CHUNK_DIAMETER *
        PC_LEGACY_VERTICAL_CHUNK_COUNT);
    if (s_candidates.capacity() < fixedRendererCapacity)
        s_candidates.reserve(fixedRendererCapacity);

    s_candidates.clear();
    ++s_statCalls;
    s_statPending += static_cast<long>(pending.size());
    for (WorldRenderer *candidate : pending)
    {
        if (candidate == nullptr || !candidate->needsUpdate)
            continue;

        const bool active = candidate->isTerrainBuildInProgress();
        const bool inFrustum = candidate->isInFrustum;
        if (frustumOnly && !inFrustum && !active)
            continue;

        const float distance = viewer != nullptr
            ? candidate->distanceToEntitySquared(viewer)
            : 0.0f;

#if PC_LEGACY_DEFER_MESH_DURING_POPULATE
        // Keep an already-published distant mesh visible while incremental
        // population can still mutate this section. Nearby sections remain
        // immediate so block interaction and close terrain edits stay responsive.
        // Unpublished sections are always allowed one provisional build so a new
        // chunk never becomes a visible hole just because decoration is pending.
        if (!active && candidate->hasPublishedTerrain() && candidate->worldObj != nullptr &&
            distance > PC_LEGACY_POPULATE_MESH_DEFER_DISTANCE_SQ)
        {
            const int_t chunkX = JavaArithmetic::intShr(candidate->posX, 4);
            const int_t chunkZ = JavaArithmetic::intShr(candidate->posZ, 4);
            if (candidate->worldObj->isChunkPopulationPendingForRendering(chunkX, chunkZ))
                continue;
        }
#endif
        s_candidates.push_back({candidate, distance, active, inFrustum});
    }

    if (s_candidates.empty())
        return;

    auto priority = [](const PcLegacyMeshCandidate &a, const PcLegacyMeshCandidate &b)
    {
        const bool aActive = a.active;
        const bool bActive = b.active;
        if (aActive != bActive)
            return aActive;

        if (a.inFrustum != b.inFrustum)
            return a.inFrustum;

        if (a.distance != b.distance)
            return a.distance < b.distance;

        return a.renderer->chunkIndex < b.renderer->chunkIndex;
    };

    const std::size_t candidateCount = std::min<std::size_t>(
        s_candidates.size(), static_cast<std::size_t>(PC_LEGACY_RENDERER_UPDATE_CANDIDATES_PER_FRAME));
    if (candidateCount > 0)
    {
        std::partial_sort(
            s_candidates.begin(),
            s_candidates.begin() + candidateCount,
            s_candidates.end(),
            priority);
    }

    PcLegacyFrameMeshBudget budget(requestedUpdateLimit);
    std::size_t candidateIndex = 0;
    WorldRenderer *current = nullptr;

    while (!budget.exhausted())
    {
        if (current == nullptr)
        {
            while (candidateIndex < candidateCount)
            {
                WorldRenderer *nextCandidate = s_candidates[candidateIndex++].renderer;
                if (nextCandidate != nullptr && nextCandidate->needsUpdate)
                {
                    current = nextCandidate;
                    break;
                }
            }
            if (current == nullptr)
                break;
        }

        budget.run(current);

        if (!current->needsUpdate)
        {
            current = nullptr;
            continue;
        }

        if (current->isTerrainBuildInProgress())
            continue;

        // Dependency-gated or otherwise non-active work gets one attempt per
        // frame pass. This prevents a missing source chunk from consuming the
        // entire Legacy meshing budget while another visible section is ready.
        current = nullptr;
    }
}

#endif
