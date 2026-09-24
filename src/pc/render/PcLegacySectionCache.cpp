#include "pc/render/PcLegacySectionCache.h"

#if PLATFORM_INCREMENTAL_TERRAIN_BUILD

#include <algorithm>

#include "net/minecraft/src/BiomeGenBase.h"
#include "net/minecraft/src/Block.h"
#include "net/minecraft/src/ChunkCache.h"
#include "net/minecraft/src/ExtendedBlockStorage.h"
#include "net/minecraft/src/Material.h"
#include "net/minecraft/src/NibbleArray.h"
#include "net/minecraft/src/World.h"
#include "net/minecraft/src/WorldProvider.h"
#include "pc/render/PcLegacyBlockRenderInfo.h"
#include "pc/tuning/PcLegacyTuning.h"

PcLegacySectionCache::PcLegacySectionCache(ChunkCache *sourceIn, World *worldIn,
    int_t minXIn, int_t minYIn, int_t minZIn)
    : source(sourceIn), world(worldIn), minX(minXIn), minY(minYIn), minZ(minZIn)
{
    preloadBlockIds();
    precomputeSectionData();
}

void PcLegacySectionCache::beginStep()
{
    // Scalar brightness includes the world's current skylight subtraction.
    // Packed sky/block light and block data stay stable for the lifetime of a
    // partial build; block changes invalidate the WorldRenderer and restart it.
    lightValueValid.fill(0);
}

int PcLegacySectionCache::cellIndex(int_t x, int_t y, int_t z) const
{
    const int_t lx = x - minX;
    const int_t ly = y - minY;
    const int_t lz = z - minZ;
    if (lx < 0 || lx >= kSize || ly < 0 || ly >= kSize || lz < 0 || lz >= kSize)
        return -1;
    return static_cast<int>((ly * kSize + lz) * kSize + lx);
}

int PcLegacySectionCache::horizontalIndex(int_t x, int_t z) const
{
    const int_t lx = x - minX;
    const int_t lz = z - minZ;
    if (lx < 0 || lx >= kSize || lz < 0 || lz >= kSize)
        return -1;
    return static_cast<int>(lz * kSize + lx);
}

void PcLegacySectionCache::preloadBlockIds()
{
    if (source == nullptr)
        return;

#if PC_LEGACY_DIRECT_SECTION_SNAPSHOT
    const int_t sectionX = minX + 1;
    const int_t sectionY = minY + 1;
    const int_t sectionZ = minZ + 1;
    const bool sectionAligned = (sectionX & 15) == 0 && (sectionY & 15) == 0 && (sectionZ & 15) == 0;
    const ExtendedBlockStorage *storage = sectionAligned
        ? source->getResidentBlockStorageAt(sectionX, sectionY, sectionZ)
        : nullptr;
    const std::vector<byte_t> *blockLsb = storage != nullptr ? &storage->func_48692_g() : nullptr;
    const NibbleArray *blockMsb = storage != nullptr ? storage->getBlockMSBArray() : nullptr;
#endif

    int index = 0;
    for (int_t y = 0; y < kSize; ++y)
    {
        for (int_t z = 0; z < kSize; ++z)
        {
            for (int_t x = 0; x < kSize; ++x, ++index)
            {
                int_t id = 0;
#if PC_LEGACY_DIRECT_SECTION_SNAPSHOT
                const bool centralCell = sectionAligned && x > 0 && x < 17 && y > 0 && y < 17 && z > 0 && z < 17;
                if (centralCell)
                {
                    if (blockLsb != nullptr)
                    {
                        const int_t localX = x - 1;
                        const int_t localY = y - 1;
                        const int_t localZ = z - 1;
                        const int_t storageIndex = (localY << 8) | (localZ << 4) | localX;
                        id = static_cast<int_t>((*blockLsb)[static_cast<std::size_t>(storageIndex)]) & 0xff;
                        if (blockMsb != nullptr)
                            id |= blockMsb->get(localX, localY, localZ) << 8;
                    }
                }
                else
#endif
                {
                    id = source->getBlockId(minX + x, minY + y, minZ + z);
                }
                if (id < 0 || id >= Block::BLOCK_REGISTRY_SIZE)
                    id = 0;
                blockIds[static_cast<std::size_t>(index)] = static_cast<std::uint16_t>(id);
            }
        }
    }
}

void PcLegacySectionCache::precomputeSectionData()
{
    std::array<std::uint8_t, Block::BLOCK_REGISTRY_SIZE> opaqueById{};
    for (int_t id = 1; id < Block::BLOCK_REGISTRY_SIZE; ++id)
    {
        Block *block = Block::blocksList[id];
        if (block == nullptr)
            continue;
        opaqueById[static_cast<std::size_t>(id)] =
            (Block::staticOpaqueCubeLookupSafe[id] ? Block::opaqueCubeLookup[id] : block->isOpaqueCube()) ? 1u : 0u;
    }

    std::array<std::uint8_t, kSectionBlockCount> opaqueCells{};
    int opaqueCount = 0;

    for (int_t localY = 0; localY < 16; ++localY)
    {
        const int cacheY = localY + 1;
        for (int_t localZ = 0; localZ < 16; ++localZ)
        {
            const int cacheZ = localZ + 1;
            int index = (cacheY * kSize + cacheZ) * kSize + 1;
            for (int_t localX = 0; localX < 16; ++localX, ++index)
            {
                const int_t blockId = static_cast<int_t>(blockIds[static_cast<std::size_t>(index)]);
                if (blockId <= 0 || blockId >= Block::BLOCK_REGISTRY_SIZE || Block::blocksList[blockId] == nullptr)
                    continue;

                const int_t localBlockIndex = (localY << 8) | (localZ << 4) | localX;
                if (opaqueById[static_cast<std::size_t>(blockId)] != 0)
                {
                    opaqueCells[static_cast<std::size_t>(localBlockIndex)] = 1;
                    ++opaqueCount;
                }

                const PcLegacyBlockRenderInfo &blockInfo = pcLegacyGetBlockRenderInfo(blockId);

                std::uint8_t mask = 0;
                if (blockInfo.simpleOpaqueCube)
                {
                    if (!opaqueById[blockIds[static_cast<std::size_t>(index - kHorizontalCellCount)]]) mask |= 1u << 0;
                    if (!opaqueById[blockIds[static_cast<std::size_t>(index + kHorizontalCellCount)]]) mask |= 1u << 1;
                    if (!opaqueById[blockIds[static_cast<std::size_t>(index - kSize)]]) mask |= 1u << 2;
                    if (!opaqueById[blockIds[static_cast<std::size_t>(index + kSize)]]) mask |= 1u << 3;
                    if (!opaqueById[blockIds[static_cast<std::size_t>(index - 1)]]) mask |= 1u << 4;
                    if (!opaqueById[blockIds[static_cast<std::size_t>(index + 1)]]) mask |= 1u << 5;
                    exposedFaceMask[static_cast<std::size_t>(index)] = mask;
                }

#if PC_LEGACY_RENDER_WORKSETS
                const int_t pass = blockInfo.renderPass != 0 ? 1 : 0;
                const bool renderCandidate = !blockInfo.simpleOpaqueCube || mask != 0;
                const bool needsTileEntityScan = Block::isBlockContainer[blockId];
                if (needsTileEntityScan)
                {
                    const int_t workIndex = renderWorkCounts[0]++;
                    renderWorksets[0][static_cast<std::size_t>(workIndex)] =
                        static_cast<std::uint16_t>(localBlockIndex);
                }
                if (renderCandidate && (pass != 0 || !needsTileEntityScan))
                {
                    const int_t workIndex = renderWorkCounts[static_cast<std::size_t>(pass)]++;
                    renderWorksets[static_cast<std::size_t>(pass)][static_cast<std::size_t>(workIndex)] =
                        static_cast<std::uint16_t>(localBlockIndex);
                }
#endif
            }
        }
    }

#if !PC_LEGACY_RENDER_WORKSETS
    for (int_t localIndex = 0; localIndex < kSectionBlockCount; ++localIndex)
    {
        const int_t localX = localIndex & 15;
        const int_t localZ = (localIndex >> 4) & 15;
        const int_t localY = (localIndex >> 8) & 15;
        const int_t id = getBlockIdLocal(localX, localY, localZ);
        if (id <= 0 || id >= Block::BLOCK_REGISTRY_SIZE || Block::blocksList[id] == nullptr)
            continue;
        const int_t pass = pcLegacyGetBlockRenderInfo(id).renderPass != 0 ? 1 : 0;
        const int_t workIndex = renderWorkCounts[static_cast<std::size_t>(pass)]++;
        renderWorksets[static_cast<std::size_t>(pass)][static_cast<std::size_t>(workIndex)] =
            static_cast<std::uint16_t>(localIndex);
    }
#endif

#if PC_LEGACY_CPU_SECTION_OCCLUSION
    visibility.build(opaqueCells, opaqueCount);
#endif
}

int_t PcLegacySectionCache::getRenderWorkCount(int_t pass) const
{
    if (pass < 0 || pass > 1)
        return 0;
    return renderWorkCounts[static_cast<std::size_t>(pass)];
}

std::uint16_t PcLegacySectionCache::getRenderWorkEntry(int_t pass, int_t index) const
{
    if (pass < 0 || pass > 1 || index < 0 || index >= getRenderWorkCount(pass))
        return 0;
    return renderWorksets[static_cast<std::size_t>(pass)][static_cast<std::size_t>(index)];
}

bool PcLegacySectionCache::hasRenderWork() const
{
    return renderWorkCounts[0] > 0 || renderWorkCounts[1] > 0;
}

std::uint8_t PcLegacySectionCache::getVisibleFacesFrom(int_t face) const
{
#if PC_LEGACY_CPU_SECTION_OCCLUSION
    return visibility.visibleFacesFrom(face);
#else
    (void)face;
    return PcLegacySectionVisibility::kAllFaces;
#endif
}

int_t PcLegacySectionCache::getBlockId(int_t x, int_t y, int_t z)
{
    const int index = cellIndex(x, y, z);
    if (index < 0)
        return source != nullptr ? source->getBlockId(x, y, z) : 0;
    return static_cast<int_t>(blockIds[static_cast<std::size_t>(index)]);
}

TileEntity *PcLegacySectionCache::getBlockTileEntity(int_t x, int_t y, int_t z)
{
    return source != nullptr ? source->getBlockTileEntity(x, y, z) : nullptr;
}

int_t PcLegacySectionCache::getLightBrightnessForSkyBlocks(int_t x, int_t y, int_t z, int_t minimumBlockLight)
{
    const int index = cellIndex(x, y, z);
    if (index < 0 || source == nullptr)
        return source != nullptr ? source->getLightBrightnessForSkyBlocks(x, y, z, minimumBlockLight) : 0;

    const std::size_t cell = static_cast<std::size_t>(index);
    if (!packedLightValid[cell])
    {
        packedLight[cell] = source->getLightBrightnessForSkyBlocks(x, y, z, 0);
        packedLightValid[cell] = 1;
    }

    const int_t base = packedLight[cell];
    const int_t skyLight = (base >> 20) & 0xf;
    int_t blockLight = (base >> 4) & 0xf;
    if (blockLight < minimumBlockLight)
        blockLight = minimumBlockLight;
    return (skyLight << 20) | (blockLight << 4);
}

int_t PcLegacySectionCache::getCachedLightValue(int_t x, int_t y, int_t z, int index)
{
    if (index < 0 || source == nullptr)
        return source != nullptr ? source->getLightValue(x, y, z) : 0;

    const std::size_t cell = static_cast<std::size_t>(index);
    if (!lightValueValid[cell])
    {
        int_t value = source->getLightValue(x, y, z);
        if (value < 0) value = 0;
        if (value > 15) value = 15;
        lightValue[cell] = static_cast<std::uint8_t>(value);
        lightValueValid[cell] = 1;
    }
    return static_cast<int_t>(lightValue[cell]);
}

float PcLegacySectionCache::getBrightness(int_t x, int_t y, int_t z, int_t minimumLight)
{
    if (world == nullptr || world->worldProvider == nullptr)
        return source != nullptr ? source->getBrightness(x, y, z, minimumLight) : 0.0f;

    int_t light = getCachedLightValue(x, y, z, cellIndex(x, y, z));
    if (light < minimumLight)
        light = minimumLight;
    if (light < 0) light = 0;
    if (light > 15) light = 15;
    return world->worldProvider->lightBrightnessTable[light];
}

float PcLegacySectionCache::getLightBrightness(int_t x, int_t y, int_t z)
{
    if (world == nullptr || world->worldProvider == nullptr)
        return source != nullptr ? source->getLightBrightness(x, y, z) : 0.0f;

    int_t light = getCachedLightValue(x, y, z, cellIndex(x, y, z));
    if (light < 0) light = 0;
    if (light > 15) light = 15;
    return world->worldProvider->lightBrightnessTable[light];
}

int_t PcLegacySectionCache::getBlockMetadata(int_t x, int_t y, int_t z)
{
    const int index = cellIndex(x, y, z);
    if (index < 0 || source == nullptr)
        return source != nullptr ? source->getBlockMetadata(x, y, z) : 0;

    const std::size_t cell = static_cast<std::size_t>(index);
    if (!metadataValid[cell])
    {
        metadata[cell] = static_cast<std::uint8_t>(source->getBlockMetadata(x, y, z) & 0xf);
        metadataValid[cell] = 1;
    }
    return static_cast<int_t>(metadata[cell]);
}

Material *PcLegacySectionCache::getBlockMaterial(int_t x, int_t y, int_t z)
{
    const int_t id = getBlockId(x, y, z);
    if (id <= 0 || id >= Block::BLOCK_REGISTRY_SIZE || Block::blocksList[id] == nullptr)
        return Material::air;
    return Block::blocksList[id]->blockMaterial;
}

bool PcLegacySectionCache::isBlockOpaqueCube(int_t x, int_t y, int_t z)
{
    const int_t id = getBlockId(x, y, z);
    if (id <= 0 || id >= Block::BLOCK_REGISTRY_SIZE)
        return false;

    Block *block = Block::blocksList[id];
    if (block == nullptr)
        return false;
    if (Block::staticOpaqueCubeLookupSafe[id])
        return Block::opaqueCubeLookup[id];
    return block->isOpaqueCube();
}

bool PcLegacySectionCache::isBlockNormalCube(int_t x, int_t y, int_t z)
{
    const int_t id = getBlockId(x, y, z);
    return id > 0 && id < Block::BLOCK_REGISTRY_SIZE &&
        pcLegacyGetBlockRenderInfo(id).normalCube;
}

std::uint8_t PcLegacySectionCache::getExposedFaceMask(int_t x, int_t y, int_t z)
{
    const int_t localX = x - minX;
    const int_t localY = y - minY;
    const int_t localZ = z - minZ;
    if (localX > 0 && localX < kSize - 1 &&
        localY > 0 && localY < kSize - 1 &&
        localZ > 0 && localZ < kSize - 1)
    {
        const int index = (localY * kSize + localZ) * kSize + localX;
        return exposedFaceMask[static_cast<std::size_t>(index)];
    }

    auto visibleAgainst = [&](int_t nx, int_t ny, int_t nz) -> bool
    {
        return !isBlockOpaqueCube(nx, ny, nz);
    };

    std::uint8_t mask = 0;
    if (visibleAgainst(x, y - 1, z)) mask |= 1u << 0;
    if (visibleAgainst(x, y + 1, z)) mask |= 1u << 1;
    if (visibleAgainst(x, y, z - 1)) mask |= 1u << 2;
    if (visibleAgainst(x, y, z + 1)) mask |= 1u << 3;
    if (visibleAgainst(x - 1, y, z)) mask |= 1u << 4;
    if (visibleAgainst(x + 1, y, z)) mask |= 1u << 5;
    return mask;
}

int_t PcLegacySectionCache::getFacePackedBrightness(int_t x, int_t y, int_t z, int_t side, int_t minimumBlockLight)
{
    switch (side)
    {
    case 0: --y; break;
    case 1: ++y; break;
    case 2: --z; break;
    case 3: ++z; break;
    case 4: --x; break;
    case 5: ++x; break;
    default: break;
    }

    const int index = cellIndex(x, y, z);
    if (index < 0 || source == nullptr)
        return source != nullptr ? source->getLightBrightnessForSkyBlocks(x, y, z, minimumBlockLight) : 0;

    const std::size_t cell = static_cast<std::size_t>(index);
    if (!packedLightValid[cell])
    {
        packedLight[cell] = source->getLightBrightnessForSkyBlocks(x, y, z, 0);
        packedLightValid[cell] = 1;
    }

    const int_t base = packedLight[cell];
    const int_t skyLight = (base >> 20) & 0xf;
    int_t blockLight = (base >> 4) & 0xf;
    if (blockLight < minimumBlockLight)
        blockLight = minimumBlockLight;
    return (skyLight << 20) | (blockLight << 4);
}


bool PcLegacySectionCache::isAirBlock(int_t x, int_t y, int_t z)
{
    const int_t id = getBlockId(x, y, z);
    return id <= 0 || id >= Block::BLOCK_REGISTRY_SIZE || Block::blocksList[id] == nullptr;
}

BiomeGenBase *PcLegacySectionCache::getBiomeGenForCoords(int_t x, int_t z)
{
    const int index = horizontalIndex(x, z);
    if (index < 0 || source == nullptr)
        return source != nullptr ? source->getBiomeGenForCoords(x, z) : nullptr;

    const std::size_t cell = static_cast<std::size_t>(index);
    if (!biomeValid[cell])
    {
        biomes[cell] = source->getBiomeGenForCoords(x, z);
        biomeValid[cell] = 1;
    }
    return biomes[cell];
}

int_t PcLegacySectionCache::getHeight()
{
    return source != nullptr ? source->getHeight() : 0;
}

bool PcLegacySectionCache::func_48452_a()
{
    return source != nullptr && source->func_48452_a();
}

WorldChunkManager *PcLegacySectionCache::getWorldChunkManager()
{
    return source != nullptr ? source->getWorldChunkManager() : nullptr;
}

#endif
