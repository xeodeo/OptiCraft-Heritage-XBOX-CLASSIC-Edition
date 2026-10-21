#pragma once

#include "ItemBlock.h"

class BlockHalfSlab;

class ItemSlab : public ItemBlock {
public:
    ItemSlab(int id, BlockHalfSlab* singleSlab, BlockHalfSlab* doubleSlab, bool isDouble);
    ItemSlab(int id);

    int getIconFromDamage(int damage) override;
    int getMetadata(int damage) override;
    std::string getItemNameIS(ItemStack* itemstack) override;
    bool onItemUse(ItemStack* itemstack, EntityPlayer* entityplayer, World* world, int i, int j, int k, int side) override;

private:
    bool tryPlaceDoubleSlab(ItemStack* itemstack, EntityPlayer* entityplayer, World* world, int i, int j, int k, int side);

    BlockHalfSlab* theHalfSlab = nullptr;
    BlockHalfSlab* doubleSlab = nullptr;
    bool isFullBlock = false;
};
