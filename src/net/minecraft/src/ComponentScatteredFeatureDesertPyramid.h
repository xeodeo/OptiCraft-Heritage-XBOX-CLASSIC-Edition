#pragma once

#include "ComponentScatteredFeature.h"

// net.minecraft.src.ComponentScatteredFeatureDesertPyramid
class ComponentScatteredFeatureDesertPyramid : public ComponentScatteredFeature
{
public:
	ComponentScatteredFeatureDesertPyramid(Random &random, int_t x, int_t z);
	~ComponentScatteredFeatureDesertPyramid() override = default;

	bool addComponentParts(World *world, Random &random, const StructureBoundingBox &chunkBounds) override;

private:
	bool chestsPlaced[4];
};
