#pragma once

#include "Block.h"

// net.minecraft.src.BlockWood
class BlockWood : public Block
{
public:
	explicit BlockWood(int_t id);
	int_t getBlockTextureFromSideAndMetadata(int_t side, int_t metadata) override;
	void getSubBlocks(int_t id, CreativeTabs *tab, std::vector<ItemStack *> &list) override;

protected:
	int_t damageDropped(int_t metadata) override;
};
