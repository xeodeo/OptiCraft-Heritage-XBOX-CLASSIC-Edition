#pragma once

#include "ShapedRecipes.h"

class InventoryCrafting;
class ItemStack;
class World;

// net.minecraft.src.RecipesMapExtending
class RecipesMapExtending : public ShapedRecipes
{
public:
    RecipesMapExtending();

    bool matches(InventoryCrafting *inv) override;
    bool matches(InventoryCrafting *inv, World *world) override;
    ItemStack *getCraftingResult(InventoryCrafting *inv) override;
};
