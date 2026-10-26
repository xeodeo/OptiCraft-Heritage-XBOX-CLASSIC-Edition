#pragma once

#include "Block.h"

// net.minecraft.src.BlockSandStone
class BlockSandStone : public Block
{
public:
	BlockSandStone(int_t i);
	int_t getBlockTextureFromSideAndMetadata(int_t side, int_t metadata) override;
	int_t getBlockTextureFromSide(int_t i) override;
	void getSubBlocks(int_t id, CreativeTabs *tab, std::vector<ItemStack *> &list) override;
protected:
	int_t damageDropped(int_t metadata) override;
};
