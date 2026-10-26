#pragma once

#include "Item.h"

class NBTTagCompound;
class World;
class EntityPlayer;
class ItemStack;

// net.minecraft.src.ItemWritableBook
class ItemWritableBook : public Item
{
public:
    explicit ItemWritableBook(int id);

    ItemStack *onItemRightClick(ItemStack *itemstack, World *world, EntityPlayer *entityplayer) override;
    bool getShareTag() override;

    static bool validBookTagPages(NBTTagCompound *nbttagcompound);
};
