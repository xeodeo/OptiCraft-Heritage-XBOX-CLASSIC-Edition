#pragma once

#include "Item.h"

class ItemBlock : public Item {
public:
    ItemBlock(int id);
    int getBlockID() const;

    bool onItemUse(ItemStack* itemstack, EntityPlayer* entityplayer, World* world, int i, int j, int k, int l) override;
    std::string getItemNameIS(ItemStack* itemstack) override;
    std::string getItemName() override;
    int getIconFromDamage(int damage) override;

protected:
    int blockID;
};
