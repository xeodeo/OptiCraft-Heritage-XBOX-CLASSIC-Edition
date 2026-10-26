#pragma once

#include "StructureStart.h"

class Random;
class World;

// net.minecraft.src.StructureScatteredFeatureStart
class StructureScatteredFeatureStart : public StructureStart
{
public:
	StructureScatteredFeatureStart(World *world, Random &random, int_t chunkX, int_t chunkZ);
	~StructureScatteredFeatureStart() override = default;
};
