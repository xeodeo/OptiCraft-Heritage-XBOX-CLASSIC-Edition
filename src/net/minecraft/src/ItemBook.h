#pragma once

#include "Item.h"

class ItemStack;

// net.minecraft.src.ItemBook (Java 1.5.2): a single book can go on the enchanting table.
class ItemBook : public Item
{
public:
    explicit ItemBook(int id);

    bool isItemTool(ItemStack *stack) override;
    int getItemEnchantability() override;
};
