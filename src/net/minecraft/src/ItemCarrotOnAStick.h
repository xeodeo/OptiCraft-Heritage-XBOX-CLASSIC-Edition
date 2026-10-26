#pragma once

#include "Item.h"

class ItemStack;
class World;
class EntityPlayer;

// net.minecraft.src.ItemCarrotOnAStick
class ItemCarrotOnAStick : public Item
{
public:
    explicit ItemCarrotOnAStick(int id);

    bool isFull3D() override;
    bool shouldRotateAroundWhenRendering() override;
    ItemStack *onItemRightClick(ItemStack *itemstack, World *world, EntityPlayer *entityplayer) override;
};
