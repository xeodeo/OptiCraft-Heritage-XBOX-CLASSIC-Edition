#pragma once

#include "Item.h"
#include <string>
#include <vector>

class NBTTagCompound;
class World;
class EntityPlayer;
class ItemStack;

// net.minecraft.src.ItemEditableBook
class ItemEditableBook : public Item
{
public:
    explicit ItemEditableBook(int id);

    static bool validBookTagContents(NBTTagCompound *nbttagcompound);

    std::string getItemDisplayName(ItemStack *itemstack) override;
    void addInformation(ItemStack *itemstack, std::vector<std::string> &information) override;
    ItemStack *onItemRightClick(ItemStack *itemstack, World *world, EntityPlayer *entityplayer) override;
    bool getShareTag() override;
    bool hasEffect(ItemStack *itemstack) override;
};
