#pragma once

#include "ItemMultiTextureTile.h"

// net.minecraft.src.ItemAnvilBlock
class ItemAnvilBlock : public ItemMultiTextureTile
{
public:
	explicit ItemAnvilBlock(Block *block);
	ItemAnvilBlock(int_t id, Block *block);
	int_t getMetadata(int_t damage) override { return damage << 2; }
};
