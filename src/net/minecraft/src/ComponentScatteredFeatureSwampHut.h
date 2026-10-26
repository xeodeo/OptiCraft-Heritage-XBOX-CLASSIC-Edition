#pragma once

#include "ComponentScatteredFeature.h"

// net.minecraft.src.ComponentScatteredFeatureSwampHut
class ComponentScatteredFeatureSwampHut : public ComponentScatteredFeature
{
public:
	ComponentScatteredFeatureSwampHut(Random &random, int_t x, int_t z);
	~ComponentScatteredFeatureSwampHut() override = default;

	bool addComponentParts(World *world, Random &random, const StructureBoundingBox &chunkBounds) override;

private:
	bool hasWitch;
};
