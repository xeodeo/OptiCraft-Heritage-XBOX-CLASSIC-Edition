#pragma once

#include "BlockOreStorage.h"

// net.minecraft.src.BlockPoweredOre
class BlockPoweredOre : public BlockOreStorage
{
public:
	BlockPoweredOre(int_t i, int_t j);
	bool canProvidePower() override;
	bool isPoweringTo(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k, int_t l) override;
};
