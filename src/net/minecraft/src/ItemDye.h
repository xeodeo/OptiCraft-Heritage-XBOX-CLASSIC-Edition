#pragma once

#include "Item.h"
#include <string>

class ItemDye : public Item {
public:
    ItemDye(int id);

    int getIconFromDamage(int damage) override;
    std::string getItemNameIS(ItemStack* itemstack) override;
    bool onItemUse(ItemStack* itemstack, EntityPlayer* entityplayer, World* world, int i, int j, int k, int l) override;
    static bool applyBonemeal(ItemStack* itemstack, World* world, int i, int j, int k);
    void useItemOnEntity(ItemStack* itemstack, EntityLiving* entityliving) override;
    void saddleEntity(ItemStack* itemstack, EntityLiving* entityliving) override;
    void getSubItems(int id, CreativeTabs *tab, std::vector<ItemStack *> &list) override;

    static const std::string dyeColors[16];
    static const int worldChunkManager[16];
};
