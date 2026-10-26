#pragma once

#include "StructureComponent.h"

// net.minecraft.src.ComponentScatteredFeature
class ComponentScatteredFeature : public StructureComponent
{
public:
	ComponentScatteredFeature(Random &random, int_t x, int_t y, int_t z,
	                          int_t sizeX, int_t sizeY, int_t sizeZ);
	~ComponentScatteredFeature() override = default;

protected:
	bool func_74935_a(World *world, const StructureBoundingBox &chunkBounds, int_t offset);

	int_t scatteredFeatureSizeX;
	int_t scatteredFeatureSizeY;
	int_t scatteredFeatureSizeZ;
	int_t field_74936_d;
};
