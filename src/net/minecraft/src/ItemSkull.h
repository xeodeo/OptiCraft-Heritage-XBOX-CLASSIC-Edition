#pragma once

#include "Item.h"

// net.minecraft.src.ItemSkull
class ItemSkull : public Item
{
public:
	static const char *const skullTypes[5];

	explicit ItemSkull(int id);

	int getIconFromDamage(int damage) override;
	int getMetadata(int damage) override;
	std::string getItemNameIS(ItemStack *itemstack) override;
	bool onItemUse(ItemStack *itemstack, EntityPlayer *entityplayer, World *world, int i, int j, int k, int side) override;
	void getSubItems(int id, CreativeTabs *tab, std::vector<ItemStack *> &list) override;
};
