#pragma once
#include "Item.h"
class ItemCarrotOnAStick : public Item { public: explicit ItemCarrotOnAStick(int id) : Item(id) { setMaxStackSize(1); setMaxDamage(25); } bool isFull3D() override { return true; } bool shouldRotateAroundWhenRendering() override { return true; } ItemStack *onItemRightClick(ItemStack *stack, World *world, EntityPlayer *player) override; };
