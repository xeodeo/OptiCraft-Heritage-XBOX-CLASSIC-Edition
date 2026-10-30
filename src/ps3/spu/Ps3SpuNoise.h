#pragma once

// Terrain noise on the SPUs (PLATFORM_SPU_NOISE, PS3 only): 4J's PerlinNoise
// SPU task (src/ps3/spu/tasks/PerlinNoise) fed from our NoiseGeneratorOctaves.
// A request is one NoiseGeneratorOctaves::generateNoiseOctaves call with
// integer origin. All requests of a batch run at once, one per SPU thread, and
// the call returns when all are done (like 4J's RandomLevelSource did with its
// five noise fields).
//
// Returns false when the batch cannot run on the SPUs (no SPU runtime, a
// request too big, verification found a mismatch...): the caller then runs
// its PPU code, which is the reference -- the SPU result is bit-for-bit the
// same or it is not used. The first PS3_SPU_NOISE_VERIFY_CALLS requests run on
// both and are compared double by double ("ps3.spu noise verify" in the log).

#include "java/Type.h"
#include "net/minecraft/src/NoiseBuffer.h"

class NoiseGeneratorOctaves;

struct Ps3SpuNoiseRequest
{
    NoiseGeneratorOctaves *generator;
    TerrainNoiseBuffer *out;
    int_t x, y, z;
    int_t sizeX, sizeY, sizeZ;
    double scaleX, scaleY, scaleZ;
};

bool ps3SpuNoiseGenerate(Ps3SpuNoiseRequest *requests, int count);
