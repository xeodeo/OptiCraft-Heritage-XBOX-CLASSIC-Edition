#pragma once

#include "Block.h"

// net.minecraft.src.BlockQuartz
class BlockQuartz : public Block
{
public:
	static const char *const quartzBlockTypes[3];

	explicit BlockQuartz(int_t i);
	int_t getBlockTextureFromSideAndMetadata(int_t side, int_t metadata) override;
	int_t getBlockTextureFromSide(int_t side) override;
	int_t damageDropped(int_t metadata) override;

protected:
	ItemStack *createStackedBlock(int_t metadata) override;
};
