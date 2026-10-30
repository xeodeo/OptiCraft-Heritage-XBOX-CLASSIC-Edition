#pragma once
// #include "Synth.h"

// 4J's ImprovedNoise_SPU (Minecraft.Client/PS3/SPU_Tasks/PerlinNoise), copied
// for the OptiCraft PS3 port. The layout is shared with the PPU, which fills it
// from NoiseGeneratorPerlin (src/ps3/spu/Ps3SpuNoise.cpp): 2048 + 4 * 8 bytes,
// no pointers, big-endian on both sides.

class ImprovedNoise_SPU// : public Synth
{
public:
    int p[512];

    double scale;
    double xo, yo, zo;


    double lerp(double t, double a, double b);
    double grad2(int hash, double x, double z);
    double grad(int hash, double x, double y, double z);

    // OptiCraft: the last argument is 1 / pow, worked out by the caller with
    // exact doublings, so the SPU never divides (see PerlinNoise_SPU.cpp).
    void add(double* buffer, double _x, double _y, double _z, int xSize, int ySize, int zSize, double xs, double ys, double zs, double invPow);
};
