#pragma once

#include "Block.h"

// net.minecraft.src.BlockStoneBrick
class BlockStoneBrick : public Block
{
public:
	explicit BlockStoneBrick(int_t id);
	int_t getBlockTextureFromSideAndMetadata(int_t side, int_t metadata) override;
	void getSubBlocks(int_t id, CreativeTabs *tab, std::vector<ItemStack *> &list) override;

protected:
	int_t damageDropped(int_t metadata) override;
};
