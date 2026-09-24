#pragma once

#include "platform/PlatformConfig.h"

#if PLATFORM_INCREMENTAL_TERRAIN_BUILD

#include <array>
#include <cstdint>

class PcLegacySectionVisibility
{
public:
    static constexpr int kSectionSize = 16;
    static constexpr int kCellCount = kSectionSize * kSectionSize * kSectionSize;
    static constexpr int kFaceCount = 6;
    static constexpr std::uint8_t kAllFaces = (1u << kFaceCount) - 1u;

    PcLegacySectionVisibility();

    void build(const std::array<std::uint8_t, kCellCount> &opaqueCells, int opaqueCount);
    std::uint8_t visibleFacesFrom(int face) const;

private:
    void setAllVisible();
    void setComponentVisible(std::uint8_t faces);

    std::array<std::uint8_t, kFaceCount> visibleByFace{};
};

#endif