#pragma once

#include "Item.h"

class ItemBucket : public Item {
public:
    ItemBucket(int id, int isFull);

    ItemStack* onItemRightClick(ItemStack* itemstack, World* world, EntityPlayer* entityplayer) override;
    bool tryPlaceContainedLiquid(World* world, double px, double py, double pz, int x, int y, int z);

    int isFull;
};
