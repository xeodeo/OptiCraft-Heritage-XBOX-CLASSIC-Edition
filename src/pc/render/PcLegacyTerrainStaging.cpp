#include "pc/render/PcLegacyTerrainStaging.h"

#if PLATFORM_INCREMENTAL_TERRAIN_BUILD

namespace
{
    PcLegacyTerrainStaging g_pcLegacyTerrainStaging;

    void clearRetainedBuffers(PcLegacyTerrainStaging &staging)
    {
        for (int pass = 0; pass < 2; ++pass)
        {
            staging.compactPassMeshes[pass].clear();
            staging.passMeshes[pass].clear();
            for (TessellatorTextureMesh &group : staging.extraTextureMeshes[pass])
                group.mesh.clear();
        }
        staging.tileEntityRenderers.clear();
        staging.staticTileEntityCandidates.clear();
    }
}

bool pcLegacyTerrainStagingAcquire(WorldRenderer *owner, World *world,
    int_t x0, int_t y0, int_t z0, int_t x1, int_t y1, int_t z1)
{
    if (owner == nullptr || world == nullptr)
        return false;
    if (g_pcLegacyTerrainStaging.owner != nullptr && g_pcLegacyTerrainStaging.owner != owner)
        return false;
    if (g_pcLegacyTerrainStaging.owner == owner)
        return true;

    clearRetainedBuffers(g_pcLegacyTerrainStaging);
    g_pcLegacyTerrainStaging.sectionCache.reset();
    g_pcLegacyTerrainStaging.sourceCache.reset();

    g_pcLegacyTerrainStaging.sourceCache.emplace(world,
        x0 - 1, y0 - 1, z0 - 1,
        x1 + 1, y1 + 1, z1 + 1);
    g_pcLegacyTerrainStaging.sectionCache.emplace(
        &*g_pcLegacyTerrainStaging.sourceCache, world, x0 - 1, y0 - 1, z0 - 1);
    g_pcLegacyTerrainStaging.owner = owner;
    return true;
}

PcLegacyTerrainStaging *pcLegacyTerrainStagingGet(WorldRenderer *owner)
{
    return owner != nullptr && g_pcLegacyTerrainStaging.owner == owner
        ? &g_pcLegacyTerrainStaging
        : nullptr;
}

void pcLegacyTerrainStagingRelease(WorldRenderer *owner)
{
    if (owner == nullptr || g_pcLegacyTerrainStaging.owner != owner)
        return;

    g_pcLegacyTerrainStaging.sectionCache.reset();
    g_pcLegacyTerrainStaging.sourceCache.reset();
    clearRetainedBuffers(g_pcLegacyTerrainStaging);
    g_pcLegacyTerrainStaging.owner = nullptr;
}

std::size_t pcLegacyTerrainStagingBytes()
{
    std::size_t bytes = 0;
    for (int pass = 0; pass < 2; ++pass)
    {
        bytes += g_pcLegacyTerrainStaging.compactPassMeshes[pass].raw.capacity() * sizeof(std::int32_t);
        bytes += g_pcLegacyTerrainStaging.passMeshes[pass].raw.capacity() * sizeof(std::int32_t);
        for (const TessellatorTextureMesh &group : g_pcLegacyTerrainStaging.extraTextureMeshes[pass])
            bytes += group.mesh.raw.capacity() * sizeof(std::int32_t);
    }
    bytes += g_pcLegacyTerrainStaging.tileEntityRenderers.capacity() * sizeof(TileEntity *);
    bytes += g_pcLegacyTerrainStaging.staticTileEntityCandidates.capacity() * sizeof(PcLegacyStaticTileEntityCandidate);
    return bytes;
}

#endif
