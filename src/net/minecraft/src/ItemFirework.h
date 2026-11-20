#pragma once

#include "Item.h"

// net.minecraft.src.ItemFirework
class ItemFirework : public Item
{
public:
    explicit ItemFirework(int id);

    bool onItemUse(ItemStack *itemstack, EntityPlayer *entityplayer, World *world,
                   int i, int j, int k, int l) override;
    void addInformation(ItemStack *itemstack, std::vector<std::string> &information) override;
};
