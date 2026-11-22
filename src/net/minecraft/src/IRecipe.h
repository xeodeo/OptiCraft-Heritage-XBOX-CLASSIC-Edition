#pragma once

#include "java/Type.h"

class InventoryCrafting;
class ItemStack;
class World;

// net.minecraft.src.IRecipe
class IRecipe
{
public:
	virtual ~IRecipe() = default;
	virtual bool matches(InventoryCrafting *inventorycrafting) = 0;
	// Java 1.5.2 matches(InventoryCrafting, World). Only recipes that read the
	// world (RecipesMapExtending) override it; world may be null.
	virtual bool matches(InventoryCrafting *inventorycrafting, World *world) { (void)world; return matches(inventorycrafting); }
	virtual ItemStack *getCraftingResult(InventoryCrafting *inventorycrafting) = 0;
	virtual int_t getRecipeSize() = 0;
	virtual ItemStack *getRecipeOutput() = 0;
};
