#pragma once

#include "ItemBlock.h"

class Block;

// net.minecraft.src.ItemSnow
class ItemSnow : public ItemBlock
{
public:
	ItemSnow(int i, Block *block);
	int getMetadata(int damage) override;
	bool onItemUse(ItemStack *itemstack, EntityPlayer *entityplayer, World *world, int i, int j, int k, int l) override;
};
