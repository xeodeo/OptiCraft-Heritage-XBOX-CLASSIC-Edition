#pragma once

#include "ItemFood.h"

// net.minecraft.src.ItemSeedFood
class ItemSeedFood : public ItemFood
{
private:
	int cropId;
	int soilId;

public:
	ItemSeedFood(int id, int healAmount, float saturationModifier, int cropId, int soilId);
	bool onItemUse(ItemStack *itemstack, EntityPlayer *entityplayer, World *world, int i, int j, int k, int l) override;
};
