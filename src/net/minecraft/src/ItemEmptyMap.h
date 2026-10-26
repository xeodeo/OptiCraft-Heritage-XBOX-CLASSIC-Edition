#pragma once

#include "ItemMapBase.h"

class ItemStack;
class World;
class EntityPlayer;

// net.minecraft.src.ItemEmptyMap
class ItemEmptyMap : public ItemMapBase
{
public:
    explicit ItemEmptyMap(int id);

    ItemStack *onItemRightClick(ItemStack *itemstack, World *world, EntityPlayer *entityplayer) override;
};
