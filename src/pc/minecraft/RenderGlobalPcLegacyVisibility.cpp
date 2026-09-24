#include "platform/PlatformConfig.h"

#if PLATFORM_INCREMENTAL_TERRAIN_BUILD

#include "net/minecraft/src/RenderGlobal.h"

#include "java/Arithmetic.h"
#include "net/minecraft/src/EntityLiving.h"
#include "net/minecraft/src/MathHelper.h"
#include "net/minecraft/src/WorldRenderer.h"
#include "pc/tuning/PcLegacyTuning.h"

namespace
{
    constexpr int kFaceCount = 6;
    constexpr int kOppositeFace[kFaceCount] = {1, 0, 3, 2, 5, 4};
    constexpr int kOffsetX[kFaceCount] = {0, 0, 0, 0, -1, 1};
    constexpr int kOffsetY[kFaceCount] = {-1, 1, 0, 0, 0, 0};
    constexpr int kOffsetZ[kFaceCount] = {0, 0, -1, 1, 0, 0};

    int positiveModulo(int value, int modulus)
    {
        int result = value % modulus;
        if (result < 0)
            result += modulus;
        return result;
    }
}

int_t RenderGlobal::pcLegacyRendererIndexAtSection(int_t sectionX, int_t sectionY, int_t sectionZ) const
{
    if (worldRenderers == nullptr || renderChunksWide <= 0 || renderChunksTall <= 0 || renderChunksDeep <= 0)
        return -1;

    const int_t ySlot = sectionY - verticalStartSection;
    if (ySlot < 0 || ySlot >= renderChunksTall)
        return -1;

    const int_t xSlot = positiveModulo(sectionX, renderChunksWide);
    const int_t zSlot = positiveModulo(sectionZ, renderChunksDeep);
    const int_t index = (zSlot * renderChunksTall + ySlot) * renderChunksWide + xSlot;
    WorldRenderer *renderer = worldRenderers[index];
    if (renderer == nullptr ||
        JavaArithmetic::intShr(renderer->posX, 4) != sectionX ||
        JavaArithmetic::intShr(renderer->posY, 4) != sectionY ||
        JavaArithmetic::intShr(renderer->posZ, 4) != sectionZ)
    {
        return -1;
    }
    return index;
}

void RenderGlobal::updatePcLegacySectionVisibility(EntityLiving *viewer)
{
#if !PC_LEGACY_CPU_SECTION_OCCLUSION
    (void)viewer;
    return;
#else
    if (worldRenderers == nullptr)
        return;

    const int_t total = renderChunksWide * renderChunksTall * renderChunksDeep;
    if (total <= 0)
        return;

    for (int_t i = 0; i < total; ++i)
    {
        if (worldRenderers[i] != nullptr)
            worldRenderers[i]->pcLegacyCpuVisible = false;
    }

    if (viewer == nullptr)
    {
        for (int_t i = 0; i < total; ++i)
        {
            if (worldRenderers[i] != nullptr)
                worldRenderers[i]->pcLegacyCpuVisible = true;
        }
        return;
    }

    const int_t cameraSectionX = JavaArithmetic::intShr(MathHelper::floor_double(viewer->posX), 4);
    const int_t cameraSectionY = JavaArithmetic::intShr(MathHelper::floor_double(viewer->posY), 4);
    const int_t cameraSectionZ = JavaArithmetic::intShr(MathHelper::floor_double(viewer->posZ), 4);
    const int_t cameraIndex = pcLegacyRendererIndexAtSection(cameraSectionX, cameraSectionY, cameraSectionZ);
    if (cameraIndex < 0)
    {
        for (int_t i = 0; i < total; ++i)
        {
            if (worldRenderers[i] != nullptr)
                worldRenderers[i]->pcLegacyCpuVisible = true;
        }
        return;
    }

    pcLegacyVisibilityEntryMasks.assign(static_cast<std::size_t>(total), 0);
    pcLegacyVisibilityQueue.clear();
    const std::size_t queueCapacity = static_cast<std::size_t>(total) * kFaceCount;
    if (pcLegacyVisibilityQueue.capacity() < queueCapacity)
        pcLegacyVisibilityQueue.reserve(queueCapacity);

    WorldRenderer *cameraRenderer = worldRenderers[cameraIndex];
    cameraRenderer->pcLegacyCpuVisible = true;

    auto enqueueNeighbour = [&](WorldRenderer *renderer, int exitFace)
    {
        const int_t sectionX = JavaArithmetic::intShr(renderer->posX, 4) + kOffsetX[exitFace];
        const int_t sectionY = JavaArithmetic::intShr(renderer->posY, 4) + kOffsetY[exitFace];
        const int_t sectionZ = JavaArithmetic::intShr(renderer->posZ, 4) + kOffsetZ[exitFace];
        const int_t neighbourIndex = pcLegacyRendererIndexAtSection(sectionX, sectionY, sectionZ);
        if (neighbourIndex < 0)
            return;

        const int entryFace = kOppositeFace[exitFace];
        const std::uint8_t entryBit = static_cast<std::uint8_t>(1u << entryFace);
        std::uint8_t &entryMask = pcLegacyVisibilityEntryMasks[static_cast<std::size_t>(neighbourIndex)];
        if ((entryMask & entryBit) != 0)
            return;

        entryMask |= entryBit;
        WorldRenderer *neighbour = worldRenderers[neighbourIndex];
        neighbour->pcLegacyCpuVisible = true;
        pcLegacyVisibilityQueue.push_back(neighbourIndex * kFaceCount + entryFace);
    };

    for (int exitFace = 0; exitFace < kFaceCount; ++exitFace)
        enqueueNeighbour(cameraRenderer, exitFace);

    std::size_t queueRead = 0;
    while (queueRead < pcLegacyVisibilityQueue.size())
    {
        const int_t encoded = pcLegacyVisibilityQueue[queueRead++];
        const int_t rendererIndex = encoded / kFaceCount;
        const int_t entryFace = encoded % kFaceCount;
        if (rendererIndex < 0 || rendererIndex >= total)
            continue;

        WorldRenderer *renderer = worldRenderers[rendererIndex];
        if (renderer == nullptr)
            continue;

        const std::uint8_t exits = renderer->pcLegacyVisibleFacesFrom(entryFace);
        for (int exitFace = 0; exitFace < kFaceCount; ++exitFace)
        {
            if ((exits & (1u << exitFace)) == 0)
                continue;
            enqueueNeighbour(renderer, exitFace);
        }
    }
#endif
}

#endif
