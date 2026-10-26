#pragma once
#include "Item.h"
#include "EntityItemFrame.h"
class ItemHangingEntity : public Item {
public:
    ItemHangingEntity(int id) : Item(id) { setMaxStackSize(64); }
    bool onItemUse(ItemStack *stack, EntityPlayer *player, World *world, int x, int y, int z, int side) override;
};
