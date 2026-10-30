#pragma once
#include "ImprovedNoise_SPU.h"

// 4J's PerlinNoise_SPU. OptiCraft: the PPU fills it from NoiseGeneratorOctaves
// (src/ps3/spu/Ps3SpuNoise.cpp) instead of 4J's setup(PerlinNoise*), which is
// why that function and the vectormath include are gone.

class PerlinNoise_SPU//: public Synth
{
public:
	static const int MAX_NOISE_LEVELS = 16;

	ImprovedNoise_SPU noiseLevels[MAX_NOISE_LEVELS];
	int levels;

	void getRegion(unsigned long long bufferEA, int x, int y, int z, int xSize, int ySize, int zSize, double xScale, double yScale, double zScale);
};
