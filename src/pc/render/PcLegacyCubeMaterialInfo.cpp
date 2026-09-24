#include "pc/render/PcLegacyCubeMaterialInfo.h"

#if PLATFORM_INCREMENTAL_TERRAIN_BUILD

#include <array>
#include <cstdint>
#include <cstring>
#include <limits>

#include "net/minecraft/src/Tessellator.h"

namespace
{
    constexpr float kAtlasSize = 256.0f;
    constexpr float kTileSize = 16.0f;
    constexpr float kAtlasUvGuard = 0.01f;

    struct PcLegacyCompactTerrainVertex
    {
        std::int16_t x;
        std::int16_t y;
        std::int16_t z;
        std::int16_t padding;
        float u;
        float v;
        std::int32_t color;
        std::int32_t brightness;
    };

    static_assert(sizeof(PcLegacyCompactTerrainVertex) == 24,
        "PC legacy compact terrain vertex layout must stay 24 bytes");

    int_t packFloatBits(float value)
    {
        int_t bits = 0;
        static_assert(sizeof(bits) == sizeof(value), "Legacy UV packing requires 32-bit floats and ints");
        std::memcpy(&bits, &value, sizeof(bits));
        return bits;
    }

    const std::array<int_t, 4> &vanillaFacePackedColors()
    {
        static const std::array<int_t, 4> colors = {
            Tessellator::packOpaqueColorLegacy(127, 127, 127),
            Tessellator::packOpaqueColorLegacy(255, 255, 255),
            Tessellator::packOpaqueColorLegacy(204, 204, 204),
            Tessellator::packOpaqueColorLegacy(153, 153, 153)
        };
        return colors;
    }

    const std::array<PcLegacyCubeUv, 256> &cubeUvTable()
    {
        static const std::array<PcLegacyCubeUv, 256> table = []()
        {
            std::array<PcLegacyCubeUv, 256> result{};
            for (int_t tile = 0; tile < 256; ++tile)
            {
                const float atlasX = static_cast<float>((tile & 0x0f) << 4);
                const float atlasY = static_cast<float>(tile & 0xf0);
                PcLegacyCubeUv &uv = result[static_cast<std::size_t>(tile)];
                uv.u0 = atlasX / kAtlasSize;
                uv.u1 = (atlasX + kTileSize - kAtlasUvGuard) / kAtlasSize;
                uv.v0 = atlasY / kAtlasSize;
                uv.v1 = (atlasY + kTileSize - kAtlasUvGuard) / kAtlasSize;
                uv.u0Bits = packFloatBits(uv.u0);
                uv.u1Bits = packFloatBits(uv.u1);
                uv.v0Bits = packFloatBits(uv.v0);
                uv.v1Bits = packFloatBits(uv.v1);
            }
            return result;
        }();
        return table;
    }

    const PcLegacyCubeUv &cubeUvForTile(int_t tile)
    {
        return cubeUvTable()[static_cast<std::size_t>(tile)];
    }
}

int_t pcLegacyGetVanillaFacePackedColor(int_t side)
{
    static constexpr std::array<int_t, 6> kSideShadeIndex = {0, 1, 2, 2, 3, 3};
    if (side < 0 || side >= 6)
        return Tessellator::packOpaqueColorLegacy(255, 255, 255);
    const int_t shadeIndex = kSideShadeIndex[static_cast<std::size_t>(side)];
    return vanillaFacePackedColors()[static_cast<std::size_t>(shadeIndex)];
}

void pcLegacySetVanillaFaceState(Tessellator &tessellator, int_t side, int_t packedBrightness)
{
    if (side < 0 || side >= 6)
    {
        tessellator.setBrightness(packedBrightness);
        return;
    }

    const int_t packedColor = pcLegacyGetVanillaFacePackedColor(side);
    if (!tessellator.setPackedFaceStateLegacy(packedColor, packedBrightness))
        tessellator.setBrightness(packedBrightness);
}

bool pcLegacyEmitCompactUnitCubeFace(RenderCapturedMesh &mesh, int_t originX, int_t originY, int_t originZ,
    int_t side, int_t x, int_t y, int_t z, int_t tile, int_t packedColor, int_t packedBrightness)
{
    if (side < 0 || side >= 6 || tile < 0 || tile >= 256)
        return false;

    const int_t localX = x - originX;
    const int_t localY = y - originY;
    const int_t localZ = z - originZ;
    if (localX < std::numeric_limits<std::int16_t>::min() || localY < std::numeric_limits<std::int16_t>::min() ||
        localZ < std::numeric_limits<std::int16_t>::min() ||
        localX >= std::numeric_limits<std::int16_t>::max() || localY >= std::numeric_limits<std::int16_t>::max() ||
        localZ >= std::numeric_limits<std::int16_t>::max())
        return false;

    if (mesh.empty())
    {
        mesh.stride = static_cast<int>(sizeof(PcLegacyCompactTerrainVertex));
        mesh.primitive = RenderPrimitive::Triangles;
        mesh.positionShort = true;
        mesh.hasTexture = true;
        mesh.texCoordOffset = 8;
        mesh.hasColor = true;
        mesh.colorOffset = 16;
        mesh.hasNormals = false;
        mesh.normalOffset = 0;
        mesh.hasBrightness = true;
        mesh.brightnessOffset = 20;
    }
    else if (mesh.stride != static_cast<int>(sizeof(PcLegacyCompactTerrainVertex)) ||
        mesh.primitive != RenderPrimitive::Triangles || !mesh.positionShort || !mesh.hasTexture ||
        mesh.texCoordOffset != 8 || !mesh.hasColor || mesh.colorOffset != 16 || mesh.hasNormals ||
        !mesh.hasBrightness || mesh.brightnessOffset != 20)
    {
        return false;
    }

    const PcLegacyCubeUv &uv = cubeUvForTile(tile);
    const std::int16_t x0 = static_cast<std::int16_t>(localX);
    const std::int16_t y0 = static_cast<std::int16_t>(localY);
    const std::int16_t z0 = static_cast<std::int16_t>(localZ);
    const std::int16_t x1 = static_cast<std::int16_t>(localX + 1);
    const std::int16_t y1 = static_cast<std::int16_t>(localY + 1);
    const std::int16_t z1 = static_cast<std::int16_t>(localZ + 1);

    std::array<PcLegacyCompactTerrainVertex, 6> vertices{};
    auto setVertex = [&](int index, std::int16_t vx, std::int16_t vy, std::int16_t vz, float u, float v)
    {
        PcLegacyCompactTerrainVertex &vertex = vertices[static_cast<std::size_t>(index)];
        vertex.x = vx;
        vertex.y = vy;
        vertex.z = vz;
        vertex.padding = 0;
        vertex.u = u;
        vertex.v = v;
        vertex.color = static_cast<std::int32_t>(packedColor);
        vertex.brightness = static_cast<std::int32_t>(packedBrightness);
    };

    switch (side)
    {
    case 0:
        setVertex(0, x0, y0, z1, uv.u0, uv.v1); setVertex(1, x0, y0, z0, uv.u0, uv.v0);
        setVertex(2, x1, y0, z0, uv.u1, uv.v0); setVertex(3, x0, y0, z1, uv.u0, uv.v1);
        setVertex(4, x1, y0, z0, uv.u1, uv.v0); setVertex(5, x1, y0, z1, uv.u1, uv.v1);
        break;
    case 1:
        setVertex(0, x1, y1, z1, uv.u1, uv.v1); setVertex(1, x1, y1, z0, uv.u1, uv.v0);
        setVertex(2, x0, y1, z0, uv.u0, uv.v0); setVertex(3, x1, y1, z1, uv.u1, uv.v1);
        setVertex(4, x0, y1, z0, uv.u0, uv.v0); setVertex(5, x0, y1, z1, uv.u0, uv.v1);
        break;
    case 2:
        setVertex(0, x0, y1, z0, uv.u1, uv.v0); setVertex(1, x1, y1, z0, uv.u0, uv.v0);
        setVertex(2, x1, y0, z0, uv.u0, uv.v1); setVertex(3, x0, y1, z0, uv.u1, uv.v0);
        setVertex(4, x1, y0, z0, uv.u0, uv.v1); setVertex(5, x0, y0, z0, uv.u1, uv.v1);
        break;
    case 3:
        setVertex(0, x0, y1, z1, uv.u0, uv.v0); setVertex(1, x0, y0, z1, uv.u0, uv.v1);
        setVertex(2, x1, y0, z1, uv.u1, uv.v1); setVertex(3, x0, y1, z1, uv.u0, uv.v0);
        setVertex(4, x1, y0, z1, uv.u1, uv.v1); setVertex(5, x1, y1, z1, uv.u1, uv.v0);
        break;
    case 4:
        setVertex(0, x0, y1, z1, uv.u1, uv.v0); setVertex(1, x0, y1, z0, uv.u0, uv.v0);
        setVertex(2, x0, y0, z0, uv.u0, uv.v1); setVertex(3, x0, y1, z1, uv.u1, uv.v0);
        setVertex(4, x0, y0, z0, uv.u0, uv.v1); setVertex(5, x0, y0, z1, uv.u1, uv.v1);
        break;
    case 5:
        setVertex(0, x1, y0, z1, uv.u0, uv.v1); setVertex(1, x1, y0, z0, uv.u1, uv.v1);
        setVertex(2, x1, y1, z0, uv.u1, uv.v0); setVertex(3, x1, y0, z1, uv.u0, uv.v1);
        setVertex(4, x1, y1, z0, uv.u1, uv.v0); setVertex(5, x1, y1, z1, uv.u0, uv.v0);
        break;
    default:
        return false;
    }

    const std::size_t oldInts = mesh.raw.size();
    const std::size_t faceInts = sizeof(vertices) / sizeof(std::int32_t);
    mesh.raw.resize(oldInts + faceInts);
    std::memcpy(mesh.raw.data() + oldInts, vertices.data(), sizeof(vertices));
    mesh.vertexCount += static_cast<int>(vertices.size());
    return true;
}

bool pcLegacyEmitUnitCubeFace(Tessellator &tessellator, int_t side, int_t x, int_t y, int_t z, int_t tile)
{
    if (tile < 0 || tile >= 256)
        return false;

    const PcLegacyCubeUv &uv = cubeUvForTile(tile);
    if (tessellator.addAxisAlignedUnitFaceWithPackedUVFast(side, static_cast<tess_coord_t>(x),
        static_cast<tess_coord_t>(y), static_cast<tess_coord_t>(z),
        uv.u0Bits, uv.u1Bits, uv.v0Bits, uv.v1Bits))
        return true;

    const float x0 = static_cast<float>(x);
    const float y0 = static_cast<float>(y);
    const float z0 = static_cast<float>(z);
    const float x1 = x0 + 1.0f;
    const float y1 = y0 + 1.0f;
    const float z1 = z0 + 1.0f;

    switch (side)
    {
    case 0:
        tessellator.addVertexWithUV(x0, y0, z1, uv.u0, uv.v1);
        tessellator.addVertexWithUV(x0, y0, z0, uv.u0, uv.v0);
        tessellator.addVertexWithUV(x1, y0, z0, uv.u1, uv.v0);
        tessellator.addVertexWithUV(x1, y0, z1, uv.u1, uv.v1);
        return true;
    case 1:
        tessellator.addVertexWithUV(x1, y1, z1, uv.u1, uv.v1);
        tessellator.addVertexWithUV(x1, y1, z0, uv.u1, uv.v0);
        tessellator.addVertexWithUV(x0, y1, z0, uv.u0, uv.v0);
        tessellator.addVertexWithUV(x0, y1, z1, uv.u0, uv.v1);
        return true;
    case 2:
        tessellator.addVertexWithUV(x0, y1, z0, uv.u1, uv.v0);
        tessellator.addVertexWithUV(x1, y1, z0, uv.u0, uv.v0);
        tessellator.addVertexWithUV(x1, y0, z0, uv.u0, uv.v1);
        tessellator.addVertexWithUV(x0, y0, z0, uv.u1, uv.v1);
        return true;
    case 3:
        tessellator.addVertexWithUV(x0, y1, z1, uv.u0, uv.v0);
        tessellator.addVertexWithUV(x0, y0, z1, uv.u0, uv.v1);
        tessellator.addVertexWithUV(x1, y0, z1, uv.u1, uv.v1);
        tessellator.addVertexWithUV(x1, y1, z1, uv.u1, uv.v0);
        return true;
    case 4:
        tessellator.addVertexWithUV(x0, y1, z1, uv.u1, uv.v0);
        tessellator.addVertexWithUV(x0, y1, z0, uv.u0, uv.v0);
        tessellator.addVertexWithUV(x0, y0, z0, uv.u0, uv.v1);
        tessellator.addVertexWithUV(x0, y0, z1, uv.u1, uv.v1);
        return true;
    case 5:
        tessellator.addVertexWithUV(x1, y0, z1, uv.u0, uv.v1);
        tessellator.addVertexWithUV(x1, y0, z0, uv.u1, uv.v1);
        tessellator.addVertexWithUV(x1, y1, z0, uv.u1, uv.v0);
        tessellator.addVertexWithUV(x1, y1, z1, uv.u0, uv.v0);
        return true;
    default:
        return false;
    }
}

#endif
