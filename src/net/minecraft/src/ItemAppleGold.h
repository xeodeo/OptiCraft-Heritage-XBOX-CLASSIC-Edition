#pragma once

#include "ItemFood.h"
#include <vector>

// net.minecraft.src.ItemAppleGold
class ItemAppleGold : public ItemFood
{
public:
	ItemAppleGold(int id, int healAmount, float saturationModifier, bool wolfFood);
	bool hasEffect(ItemStack *itemstack) override;
	const EnumRarity& getRarity(ItemStack *itemstack) override;
	ItemStack *onFoodEaten(ItemStack *itemstack, World *world, EntityPlayer *player) override;
	std::vector<int> getSubItemDamageValues() const;
	void getSubItems(int id, CreativeTabs *tab, std::vector<ItemStack *> &list) override;
};
