#pragma once

#include "BlockHalfSlab.h"

// net.minecraft.src.BlockWoodSlab
class BlockWoodSlab : public BlockHalfSlab
{
public:
	static const char *const woodType[4];

	BlockWoodSlab(int_t id, bool isDouble);

	int_t getBlockTextureFromSideAndMetadata(int_t side, int_t metadata) override;
	int_t getBlockTextureFromSide(int_t side) override;
	int_t idDropped(int_t i, Random &random) override;
	std::string getFullSlabName(int_t metadata) override;

protected:
	ItemStack *createStackedBlock(int_t metadata) override;
};
