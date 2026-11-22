#pragma once

#include "IRecipe.h"

class InventoryCrafting;
class ItemStack;

// net.minecraft.src.RecipesArmorDyes (Java 1.5.2)
class RecipesArmorDyes : public IRecipe
{
public:
	bool matches(InventoryCrafting *inventory) override;
	ItemStack *getCraftingResult(InventoryCrafting *inventory) override;
	int_t getRecipeSize() override;
	ItemStack *getRecipeOutput() override;

	static int_t computeBlendedColor(ItemStack *armorStack, int_t dyeDamage);
};
