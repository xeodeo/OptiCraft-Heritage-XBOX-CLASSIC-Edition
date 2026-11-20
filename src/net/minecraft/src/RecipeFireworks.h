#pragma once

#include <memory>

#include "IRecipe.h"

// net.minecraft.src.RecipeFireworks
class RecipeFireworks : public IRecipe
{
public:
    RecipeFireworks() = default;
    ~RecipeFireworks() override = default;

    bool matches(InventoryCrafting *inventorycrafting) override;
    ItemStack *getCraftingResult(InventoryCrafting *inventorycrafting) override;
    int_t getRecipeSize() override;
    ItemStack *getRecipeOutput() override;

private:
    std::unique_ptr<ItemStack> buildResult(InventoryCrafting *inventorycrafting) const;

    std::unique_ptr<ItemStack> cachedResult;
};
