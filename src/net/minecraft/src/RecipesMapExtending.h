#pragma once

#include "ShapedRecipes.h"

class InventoryCrafting;
class ItemStack;

// net.minecraft.src.RecipesMapExtending
class RecipesMapExtending : public ShapedRecipes
{
public:
    RecipesMapExtending();

    bool matches(InventoryCrafting *inv) override;
    ItemStack *getCraftingResult(InventoryCrafting *inv) override;
};
