#pragma once

#include "platform/PlatformConfig.h"

#if PLATFORM_INCREMENTAL_TERRAIN_BUILD

#include "java/Type.h"

class Tessellator;
struct RenderCapturedMesh;

struct PcLegacyCubeUv
{
    float u0 = 0.0f;
    float u1 = 0.0f;
    float v0 = 0.0f;
    float v1 = 0.0f;
    int_t u0Bits = 0;
    int_t u1Bits = 0;
    int_t v0Bits = 0;
    int_t v1Bits = 0;
};

int_t pcLegacyGetVanillaFacePackedColor(int_t side);
void pcLegacySetVanillaFaceState(Tessellator &tessellator, int_t side, int_t packedBrightness);
bool pcLegacyEmitCompactUnitCubeFace(RenderCapturedMesh &mesh, int_t originX, int_t originY, int_t originZ,
    int_t side, int_t x, int_t y, int_t z, int_t tile, int_t packedColor, int_t packedBrightness);
bool pcLegacyEmitUnitCubeFace(Tessellator &tessellator, int_t side, int_t x, int_t y, int_t z, int_t tile);

#endif
