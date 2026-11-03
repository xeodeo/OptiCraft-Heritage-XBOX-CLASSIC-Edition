#pragma once

#include "BlockHalfSlab.h"

// net.minecraft.src.BlockStep
class BlockStep : public BlockHalfSlab
{
public:
	BlockStep(int_t i, bool flag);
	int_t getBlockTextureFromSideAndMetadata(int_t i, int_t j) override;
	int_t getBlockTextureFromSide(int_t i) override;
	int_t idDropped(int_t i, Random &random) override;

	std::string getFullSlabName(int_t metadata) override;

	static const jstring blockStepTypes[8];
	void getSubBlocks(int_t id, CreativeTabs *tab, std::vector<ItemStack *> &list) override;

protected:
	ItemStack *createStackedBlock(int_t metadata) override;
};
