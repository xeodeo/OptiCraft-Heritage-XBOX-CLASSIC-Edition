#pragma once

#include "StructurePieceBlockSelector.h"

// net.minecraft.src.StructureScatteredFeatureStones
class StructureScatteredFeatureStones : public StructurePieceBlockSelector
{
public:
	void selectBlocks(Random &random, int_t x, int_t y, int_t z, bool isBoundary) override;
};
