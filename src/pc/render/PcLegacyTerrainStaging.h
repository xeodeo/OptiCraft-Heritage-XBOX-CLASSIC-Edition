#pragma once

#include "platform/PlatformConfig.h"

#if PLATFORM_INCREMENTAL_TERRAIN_BUILD

#include <cstddef>
#include <optional>
#include <vector>

#include "net/minecraft/src/ChunkCache.h"
#include "net/minecraft/src/Tessellator.h"
#include "pc/render/PcLegacySectionCache.h"
#include "pc/render/PcLegacyStaticTileEntityPolicy.h"

class World;
class WorldRenderer;
class TileEntity;

struct PcLegacyTerrainStaging
{
    WorldRenderer *owner = nullptr;
    std::optional<ChunkCache> sourceCache;
    std::optional<PcLegacySectionCache> sectionCache;
    RenderCapturedMesh compactPassMeshes[2];
    RenderCapturedMesh passMeshes[2];
    std::vector<TessellatorTextureMesh> extraTextureMeshes[2];
    std::vector<TileEntity *> tileEntityRenderers;
    std::vector<PcLegacyStaticTileEntityCandidate> staticTileEntityCandidates;
};

bool pcLegacyTerrainStagingAcquire(WorldRenderer *owner, World *world,
    int_t x0, int_t y0, int_t z0, int_t x1, int_t y1, int_t z1);
PcLegacyTerrainStaging *pcLegacyTerrainStagingGet(WorldRenderer *owner);
void pcLegacyTerrainStagingRelease(WorldRenderer *owner);
std::size_t pcLegacyTerrainStagingBytes();

#endif
