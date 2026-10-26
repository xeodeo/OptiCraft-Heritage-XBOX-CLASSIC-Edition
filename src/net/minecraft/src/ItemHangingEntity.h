#pragma once

#include "Item.h"

class World;
class EntityPlayer;
class ItemStack;
class EntityHanging;

class ItemHangingEntity : public Item
{
public:
    enum HangingType
    {
        TYPE_PAINTING = 0,
        TYPE_ITEM_FRAME = 1
    };

    explicit ItemHangingEntity(int id, HangingType type = TYPE_ITEM_FRAME);

    bool onItemUse(ItemStack *stack, EntityPlayer *player, World *world, int x, int y, int z, int side) override;
    bool onItemUse(ItemStack *stack, EntityPlayer *player, World *world, int x, int y, int z, int side, float hitX, float hitY, float hitZ);

private:
    EntityHanging *createHangingEntity(World *world, int x, int y, int z, int dir);

    HangingType hangingType = TYPE_ITEM_FRAME;
};
