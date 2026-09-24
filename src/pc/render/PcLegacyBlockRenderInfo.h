#pragma once

#include "platform/PlatformConfig.h"

#if PLATFORM_INCREMENTAL_TERRAIN_BUILD

#include <array>
#include <cstdint>
#include "java/Type.h"

struct PcLegacyBlockRenderInfo
{
    std::int8_t renderType = -1;
    std::uint8_t renderPass = 0;
    bool opaqueCube = false;
    bool normalCube = false;
    bool simpleOpaqueCube = false;
    bool staticTextureByMetadata = false;
    bool defaultWhiteColorMultiplier = false;
    std::array<std::array<std::int16_t, 6>, 16> textureByMetadata{};
};

const PcLegacyBlockRenderInfo &pcLegacyGetBlockRenderInfo(int_t blockId);

#endif
