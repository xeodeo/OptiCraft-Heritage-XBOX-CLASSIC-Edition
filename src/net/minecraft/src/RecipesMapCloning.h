#pragma once

#include "IRecipe.h"

class ItemStack;
class InventoryCrafting;

// net.minecraft.src.RecipesMapCloning
class RecipesMapCloning : public IRecipe
{
public:
    RecipesMapCloning() = default;
    ~RecipesMapCloning() override = default;

    bool matches(InventoryCrafting *inv) override;
    ItemStack *getCraftingResult(InventoryCrafting *inv) override;
    int_t getRecipeSize() override;
    ItemStack *getRecipeOutput() override;
};
