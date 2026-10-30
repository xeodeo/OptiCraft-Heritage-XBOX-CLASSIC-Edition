#pragma once

#include <memory>
#include <vector>

#include "NoiseBuffer.h"
#include "NoiseGenerator.h"
#include "java/Type.h"
#include "platform/PlatformConfig.h"

class NoiseGeneratorPerlin;
class Random;

// net.minecraft.src.NoiseGeneratorOctaves
class NoiseGeneratorOctaves : public NoiseGenerator
{
public:
    // skipFineOctaves leaves out that many of the highest-frequency octaves when
    // sampling. Octave i contributes with weight 2^i, so the first few are the
    // cheapest to drop: the first four of sixteen are 15/65535 of the range.
    // Every octave's permutation table is still drawn from random so the seed
    // stream seen by later generators is unchanged.
    NoiseGeneratorOctaves(Random &random, int_t octaves, int_t skipFineOctaves = 0);
    ~NoiseGeneratorOctaves() override;

    TerrainNoiseBuffer &generateNoiseOctaves(TerrainNoiseBuffer &noise, double x, double y, double z,
                                             int_t sizeX, int_t sizeY, int_t sizeZ,
                                             double scaleX, double scaleY, double scaleZ);
#if PLATFORM_FLOAT_TERRAIN_NOISE
    TerrainNoiseBuffer &generateNoiseOctavesFloat(TerrainNoiseBuffer &noise, terrain_coord_real_t x, terrain_coord_real_t y, terrain_coord_real_t z,
                                                  int_t sizeX, int_t sizeY, int_t sizeZ,
                                                  terrain_coord_real_t scaleX, terrain_coord_real_t scaleY, terrain_coord_real_t scaleZ);
#endif
    TerrainNoiseBuffer &getBiomeGenForCoords(TerrainNoiseBuffer &noise, int_t x, int_t z,
                                             int_t sizeX, int_t sizeZ,
                                             double scaleX, double scaleZ, double amplitudeScale);
#if PLATFORM_FLOAT_TERRAIN_NOISE
    TerrainNoiseBuffer &getBiomeGenForCoordsFloat(TerrainNoiseBuffer &noise, int_t x, int_t z,
                                                  int_t sizeX, int_t sizeZ,
                                                  terrain_coord_real_t scaleX, terrain_coord_real_t scaleZ,
                                                  terrain_coord_real_t amplitudeScale);
#endif

#if PLATFORM_SPU_NOISE
    // Read by the PS3 SPU noise task (src/ps3/spu/Ps3SpuNoise.cpp).
    int_t octaveCount() const { return octaves; }
    int_t firstOctaveIndex() const { return firstOctave; }
    const NoiseGeneratorPerlin *octave(int_t i) const { return generatorCollection[static_cast<std::size_t>(i)].get(); }
#endif

private:
    std::vector<std::unique_ptr<NoiseGeneratorPerlin>> generatorCollection;
    int_t octaves;
    int_t firstOctave;
};
