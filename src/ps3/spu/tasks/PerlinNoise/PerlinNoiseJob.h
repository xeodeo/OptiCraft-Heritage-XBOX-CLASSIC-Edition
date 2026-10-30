#pragma once
#include "PerlinNoise_SPU.h"

// 4J's PerlinNoise_DataIn: one noise field request, DMA'd whole into the SPU.
// OptiCraft: shared with the PPU (gcc 13, LP64), so no pointers: the output
// buffer is a 64-bit effective address. The PPU fills it in
// src/ps3/spu/Ps3SpuNoise.cpp (4J's set() lived here). Same field order as
// 4J; the sizes below are checked on both compilers.

class PerlinNoise_DataIn
{
public:
	PerlinNoise_SPU		m_perlinData;
	unsigned long long	m_outputBufferEA;	// 16-byte aligned, room for the size rounded up to 16

	int x, y, z;
	int xSize, ySize, zSize;
	double xScale, yScale, zScale;
};

// C++03 compile-time checks (the SPU compiler is gcc 4.1).
#define PS3_SPU_LAYOUT_CHECK(name, cond) typedef char ps3_spu_layout_##name[(cond) ? 1 : -1]
PS3_SPU_LAYOUT_CHECK(improved_size, sizeof(ImprovedNoise_SPU) == 2080);
PS3_SPU_LAYOUT_CHECK(perlin_size, sizeof(PerlinNoise_SPU) == 33288);
PS3_SPU_LAYOUT_CHECK(datain_size, sizeof(PerlinNoise_DataIn) == 33344);
PS3_SPU_LAYOUT_CHECK(datain_dma, (sizeof(PerlinNoise_DataIn) % 16) == 0);

// Largest field the SPU accepts (doubles), and so the PPU staging buffer size.
#define PS3_SPU_NOISE_MAX_VALUES 2048

bool PerlinNoiseJob_run(unsigned long long eaDataIn);
