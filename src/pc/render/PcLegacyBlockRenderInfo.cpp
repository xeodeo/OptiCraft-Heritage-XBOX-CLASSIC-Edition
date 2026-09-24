#include "pc/render/PcLegacyBlockRenderInfo.h"

#if PLATFORM_INCREMENTAL_TERRAIN_BUILD

#include <array>
#include <cmath>

#include "net/minecraft/src/Block.h"
#include "net/minecraft/src/Material.h"

namespace
{
    std::array<PcLegacyBlockRenderInfo, Block::BLOCK_REGISTRY_SIZE> s_renderInfo{};
    bool s_renderInfoReady = false;

    bool isUnitBounds(const Block *block)
    {
        if (block == nullptr)
            return false;
        constexpr float epsilon = 0.000001f;
        return std::fabs(static_cast<float>(block->minX)) <= epsilon &&
            std::fabs(static_cast<float>(block->minY)) <= epsilon &&
            std::fabs(static_cast<float>(block->minZ)) <= epsilon &&
            std::fabs(static_cast<float>(block->maxX) - 1.0f) <= epsilon &&
            std::fabs(static_cast<float>(block->maxY) - 1.0f) <= epsilon &&
            std::fabs(static_cast<float>(block->maxZ) - 1.0f) <= epsilon;
    }

    void buildRenderInfo()
    {
        if (s_renderInfoReady)
            return;

        for (int_t id = 0; id < Block::BLOCK_REGISTRY_SIZE; ++id)
        {
            PcLegacyBlockRenderInfo info;
            Block *block = Block::blocksList[id];
            if (block != nullptr)
            {
                const int_t renderType = block->getRenderType();
                const int_t renderPass = block->getRenderBlockPass();
                info.renderType = static_cast<std::int8_t>(renderType);
                info.renderPass = static_cast<std::uint8_t>(renderPass > 0 ? 1 : 0);
                info.opaqueCube = Block::opaqueCubeLookup[id];
                info.normalCube = block->blockMaterial != nullptr &&
                    block->blockMaterial->getIsSolid() && block->renderAsNormalBlock();
                info.simpleOpaqueCube = renderType == 0 && renderPass == 0 &&
                    info.opaqueCube && info.normalCube &&
                    Block::usesDefaultFaceCullingLookup[id] && isUnitBounds(block);
                info.staticTextureByMetadata = info.simpleOpaqueCube && block->usesDefaultWorldTextureLookup();
                info.defaultWhiteColorMultiplier = info.simpleOpaqueCube && block->usesDefaultColorMultiplier();
                if (info.staticTextureByMetadata)
                {
                    for (int_t metadata = 0; metadata < 16; ++metadata)
                    {
                        for (int_t side = 0; side < 6; ++side)
                        {
                            info.textureByMetadata[static_cast<std::size_t>(metadata)][static_cast<std::size_t>(side)] =
                                static_cast<std::int16_t>(block->getBlockTextureFromSideAndMetadata(side, metadata));
                        }
                    }
                }
            }
            s_renderInfo[static_cast<std::size_t>(id)] = info;
        }
        s_renderInfoReady = true;
    }
}

const PcLegacyBlockRenderInfo &pcLegacyGetBlockRenderInfo(int_t blockId)
{
    static const PcLegacyBlockRenderInfo emptyInfo{};
    if (blockId < 0 || blockId >= Block::BLOCK_REGISTRY_SIZE)
        return emptyInfo;
    buildRenderInfo();
    return s_renderInfo[static_cast<std::size_t>(blockId)];
}

#endif
