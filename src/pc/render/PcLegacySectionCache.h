#pragma once

#include "platform/PlatformConfig.h"

#if PLATFORM_INCREMENTAL_TERRAIN_BUILD

#include <array>
#include <cstdint>

#include "net/minecraft/src/IBlockAccess.h"
#include "pc/render/PcLegacySectionVisibility.h"

class ChunkCache;
class World;

class PcLegacySectionCache : public IBlockAccess
{
public:
    static constexpr int kSize = 18;
    static constexpr int kHorizontalCellCount = kSize * kSize;
    static constexpr int kCellCount = kSize * kSize * kSize;
    static constexpr int kSectionBlockCount = 16 * 16 * 16;

    PcLegacySectionCache(ChunkCache *source, World *world, int_t minX, int_t minY, int_t minZ);
#if PLATFORM_THREADED_TERRAIN_MESH
    // Threaded meshing (Ps3MeshWorker.cpp). The main thread constructs the cache
    // with deferSectionData (block ids only), adds everything else meshing reads
    // with captureSnapshot() and hands it to the worker; from then on the cache
    // is "detached" and never touches the ChunkCache or the World again. The
    // worker runs precomputeDeferredSectionData() and meshes from the copies.
    PcLegacySectionCache(ChunkCache *source, World *world, int_t minX, int_t minY, int_t minZ,
        bool deferSectionData);
    // Optional per-part timing of the copy (microseconds), for the PS3 log.
    struct SnapshotTimings
    {
        std::uint64_t metaUs = 0;
        std::uint64_t lightUs = 0;
        std::uint64_t biomeUs = 0;
    };
    void captureSnapshot(SnapshotTimings *timings = nullptr);
    bool isDetached() const { return detached; }
    void precomputeDeferredSectionData() { precomputeSectionData(); }
    // Whether a scalar light read saw skylight (the snapshot's Chunk::isLit).
    bool snapshotSkyLit() const { return snapshotSawSky; }
    // Reads that fell outside the snapshot and were answered from its edge.
    int snapshotOutOfRangeReads() const { return outOfRangeReads; }
    // Raw snapshot arrays for the SPU mesher (src/ps3/spu/Ps3SpuChunkMesh.cpp);
    // each is 16-byte aligned for DMA.
    const std::uint16_t *rawBlockIds() const { return blockIds.data(); }
    const std::uint8_t *rawMetadata() const { return metadata.data(); }
    const std::uint8_t *rawSavedLight() const { return savedLight.data(); }
    const std::uint16_t *rawRenderWorkset(int_t pass) const { return renderWorksets[pass != 0 ? 1 : 0].data(); }
    int_t originX() const { return minX; }
    int_t originY() const { return minY; }
    int_t originZ() const { return minZ; }
#endif

    void beginStep();

    int_t getBlockId(int_t x, int_t y, int_t z) override;
    int_t getBlockIdLocal(int_t localX, int_t localY, int_t localZ) const
    {
        const int index = ((localY + 1) * kSize + (localZ + 1)) * kSize + (localX + 1);
        return static_cast<int_t>(blockIds[static_cast<std::size_t>(index)]);
    }
    int_t getRenderWorkCount(int_t pass) const;
    std::uint16_t getRenderWorkEntry(int_t pass, int_t index) const;
    bool hasRenderWork() const;
    std::uint8_t getVisibleFacesFrom(int_t face) const;
    TileEntity *getBlockTileEntity(int_t x, int_t y, int_t z) override;
    int_t getLightBrightnessForSkyBlocks(int_t x, int_t y, int_t z, int_t minimumBlockLight) override;
    float getBrightness(int_t x, int_t y, int_t z, int_t minimumLight) override;
    float getLightBrightness(int_t x, int_t y, int_t z) override;
    int_t getBlockMetadata(int_t x, int_t y, int_t z) override;
    Material *getBlockMaterial(int_t x, int_t y, int_t z) override;
    bool isBlockOpaqueCube(int_t x, int_t y, int_t z) override;
    bool isBlockNormalCube(int_t x, int_t y, int_t z) override;
    bool isAirBlock(int_t x, int_t y, int_t z) override;
    std::uint8_t getExposedFaceMask(int_t x, int_t y, int_t z);
    std::uint8_t getExposedFaceMaskLocal(int_t localX, int_t localY, int_t localZ) const
    {
        const int index = ((localY + 1) * kSize + (localZ + 1)) * kSize + (localX + 1);
        return exposedFaceMask[static_cast<std::size_t>(index)];
    }
    int_t getFacePackedBrightness(int_t x, int_t y, int_t z, int_t side, int_t minimumBlockLight);
    BiomeGenBase *getBiomeGenForCoords(int_t x, int_t z) override;
    int_t getHeight() override;
    bool func_48452_a() override;
    WorldChunkManager *getWorldChunkManager() override;

private:
    int cellIndex(int_t x, int_t y, int_t z) const;
    int horizontalIndex(int_t x, int_t z) const;
    void preloadBlockIds();
    void precomputeSectionData();
    int_t getCachedLightValue(int_t x, int_t y, int_t z, int index);

    ChunkCache *source;
    World *world;
    int_t minX;
    int_t minY;
    int_t minZ;

#if PLATFORM_THREADED_TERRAIN_MESH
    // 16-byte aligned: the SPU mesher DMAs these straight from here.
#  define PC_LEGACY_SNAPSHOT_ALIGN alignas(16)
#else
#  define PC_LEGACY_SNAPSHOT_ALIGN
#endif
    PC_LEGACY_SNAPSHOT_ALIGN std::array<std::uint16_t, kCellCount> blockIds{};
    std::array<std::uint8_t, kCellCount> exposedFaceMask{};
    PC_LEGACY_SNAPSHOT_ALIGN std::array<std::array<std::uint16_t, kSectionBlockCount>, 2> renderWorksets{};
    std::array<int_t, 2> renderWorkCounts{};
    PcLegacySectionVisibility visibility;
    PC_LEGACY_SNAPSHOT_ALIGN std::array<std::uint8_t, kCellCount> metadata{};
    std::array<std::uint8_t, kCellCount> metadataValid{};
    std::array<int_t, kCellCount> packedLight{};
    std::array<std::uint8_t, kCellCount> packedLightValid{};
    std::array<std::uint8_t, kCellCount> lightValue{};
    std::array<std::uint8_t, kCellCount> lightValueValid{};
    std::array<BiomeGenBase *, kHorizontalCellCount> biomes{};
    std::array<std::uint8_t, kHorizontalCellCount> biomeValid{};

#if PLATFORM_THREADED_TERRAIN_MESH
    // Light is copied one block wider than the rest (20^3, like the Legacy
    // Console Edition's ChunkRebuildData): a slab or stair on the 18^3 border
    // takes its brightness from its neighbours, one block further out.
    static constexpr int kLightSize = kSize + 2;
    static constexpr int kLightCellCount = kLightSize * kLightSize * kLightSize;

    int clampedCellIndex(int_t x, int_t y, int_t z);
    int clampedHorizontalIndex(int_t x, int_t z);
    int lightCellIndex(int_t x, int_t y, int_t z);
    // ChunkCache::getSpecialBlockBrightness / getSkyBlockTypeBrightness /
    // getLightValueExt over the copies, same rules and clamps.
    int_t snapshotSavedLight(bool sky, int_t x, int_t y, int_t z);
    int_t snapshotSkyBlockTypeBrightness(bool sky, int_t x, int_t y, int_t z);
    int_t snapshotLightValue(int_t x, int_t y, int_t z, bool neighbourRule);
    int_t snapshotPackedLight(int_t x, int_t y, int_t z, int index);

    bool detached = false;
    bool snapshotSawSky = false;
    int outOfRangeReads = 0;
    int_t snapshotSkylightSubtracted = 0;
    bool snapshotHasNoSky = false;
    bool snapshotLevelsEmpty = true;
    int_t snapshotHeight = 0;
    std::array<float, 16> snapshotBrightnessTable{};
    // Saved sky light << 4 | saved block light, as getSavedLightValue answers
    // (defaults for a missing chunk or section).
    PC_LEGACY_SNAPSHOT_ALIGN std::array<std::uint8_t, kLightCellCount> savedLight{};
    // ChunkCache's fast path has a section for this cell (else scalar light is
    // the open-sky value).
    std::array<std::uint8_t, kLightCellCount> sectionPresent{};
#endif
};

#endif
