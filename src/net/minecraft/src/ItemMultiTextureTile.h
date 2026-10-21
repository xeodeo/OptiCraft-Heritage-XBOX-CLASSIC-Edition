#pragma once

#include "ItemBlock.h"

// net.minecraft.src.ItemMultiTextureTile
class ItemMultiTextureTile : public ItemBlock
{
private:
	Block *theBlock;
	const char *const *field_82804_b;
	int_t field_82804_b_count;

public:
	ItemMultiTextureTile(int_t i, Block *block, const char *const *names, int_t nameCount);
	int_t getIconFromDamage(int_t i) override;
	int_t getMetadata(int_t i) override;
	std::string getItemNameIS(ItemStack *itemstack) override;
};
