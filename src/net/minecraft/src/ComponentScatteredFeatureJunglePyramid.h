#pragma once

#include "ComponentScatteredFeature.h"
#include "StructureScatteredFeatureStones.h"

// net.minecraft.src.ComponentScatteredFeatureJunglePyramid
class ComponentScatteredFeatureJunglePyramid : public ComponentScatteredFeature
{
public:
	ComponentScatteredFeatureJunglePyramid(Random &random, int_t x, int_t z);
	~ComponentScatteredFeatureJunglePyramid() override = default;

	bool addComponentParts(World *world, Random &random, const StructureBoundingBox &chunkBounds) override;

private:
	bool placedMainChest;
	bool placedHiddenChest;
	bool placedTrap1;
	bool placedTrap2;

	static StructureScatteredFeatureStones junglePyramidsRandomScatteredStones;
};
