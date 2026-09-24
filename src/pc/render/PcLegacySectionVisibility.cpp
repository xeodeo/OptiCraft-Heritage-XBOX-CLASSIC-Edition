#include "pc/render/PcLegacySectionVisibility.h"

#if PLATFORM_INCREMENTAL_TERRAIN_BUILD

#include <array>

namespace
{
    constexpr int kVisibilityShortcutOpaqueCount = 256;

    int localIndex(int x, int y, int z)
    {
        return (y << 8) | (z << 4) | x;
    }

    std::uint8_t boundaryFaces(int x, int y, int z)
    {
        std::uint8_t faces = 0;
        if (y == 0) faces |= 1u << 0;
        if (y == 15) faces |= 1u << 1;
        if (z == 0) faces |= 1u << 2;
        if (z == 15) faces |= 1u << 3;
        if (x == 0) faces |= 1u << 4;
        if (x == 15) faces |= 1u << 5;
        return faces;
    }
}

PcLegacySectionVisibility::PcLegacySectionVisibility()
{
    setAllVisible();
}

void PcLegacySectionVisibility::build(
    const std::array<std::uint8_t, kCellCount> &opaqueCells, int opaqueCount)
{
    visibleByFace.fill(0);

    if (opaqueCount <= 0 || opaqueCount < kVisibilityShortcutOpaqueCount)
    {
        setAllVisible();
        return;
    }
    if (opaqueCount >= kCellCount)
        return;

    std::array<std::uint8_t, kCellCount> visited{};
    std::array<std::uint16_t, kCellCount> queue{};

    for (int y = 0; y < kSectionSize; ++y)
    {
        for (int z = 0; z < kSectionSize; ++z)
        {
            for (int x = 0; x < kSectionSize; ++x)
            {
                const std::uint8_t startFaces = boundaryFaces(x, y, z);
                if (startFaces == 0)
                    continue;

                const int startIndex = localIndex(x, y, z);
                if (opaqueCells[static_cast<std::size_t>(startIndex)] ||
                    visited[static_cast<std::size_t>(startIndex)])
                {
                    continue;
                }

                int queueRead = 0;
                int queueWrite = 0;
                std::uint8_t componentFaces = 0;
                queue[static_cast<std::size_t>(queueWrite++)] = static_cast<std::uint16_t>(startIndex);
                visited[static_cast<std::size_t>(startIndex)] = 1;

                while (queueRead < queueWrite)
                {
                    const int index = queue[static_cast<std::size_t>(queueRead++)];
                    const int cellX = index & 15;
                    const int cellZ = (index >> 4) & 15;
                    const int cellY = (index >> 8) & 15;
                    componentFaces |= boundaryFaces(cellX, cellY, cellZ);

                    const int neighbours[6] = {
                        index - 256, index + 256,
                        index - 16, index + 16,
                        index - 1, index + 1
                    };
                    const bool valid[6] = {
                        cellY > 0, cellY < 15,
                        cellZ > 0, cellZ < 15,
                        cellX > 0, cellX < 15
                    };

                    for (int side = 0; side < 6; ++side)
                    {
                        if (!valid[side])
                            continue;
                        const int neighbour = neighbours[side];
                        const std::size_t neighbourCell = static_cast<std::size_t>(neighbour);
                        if (opaqueCells[neighbourCell] || visited[neighbourCell])
                            continue;
                        visited[neighbourCell] = 1;
                        queue[static_cast<std::size_t>(queueWrite++)] = static_cast<std::uint16_t>(neighbour);
                    }
                }

                setComponentVisible(componentFaces);
            }
        }
    }
}

std::uint8_t PcLegacySectionVisibility::visibleFacesFrom(int face) const
{
    if (face < 0 || face >= kFaceCount)
        return kAllFaces;
    return visibleByFace[static_cast<std::size_t>(face)];
}

void PcLegacySectionVisibility::setAllVisible()
{
    visibleByFace.fill(kAllFaces);
}

void PcLegacySectionVisibility::setComponentVisible(std::uint8_t faces)
{
    for (int face = 0; face < kFaceCount; ++face)
    {
        if ((faces & (1u << face)) != 0)
            visibleByFace[static_cast<std::size_t>(face)] |= faces;
    }
}

#endif