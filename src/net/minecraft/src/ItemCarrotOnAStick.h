#pragma once
#include "Item.h"
class ItemCarrotOnAStick : public Item {
public:
    explicit ItemCarrotOnAStick(int id) : Item(id) { setMaxStackSize(1); setMaxDamage(25); }
    bool isFull3D() override { return true; }
    bool shouldRotateAroundWhenRendering() override { return true; }
    ItemStack *onItemRightClick(ItemStack *stack, World *world, EntityPlayer *player) override;
    bool canBoost(ItemStack *stack, EntityPlayer *player) const;
    int boostCost(ItemStack *stack) const;
    int remainingUses(ItemStack *stack) const;
    ItemStack *convertToFishingRod(ItemStack *stack) const;
    ItemStack *copyWithDamage(ItemStack *stack, int damage) const;
};
