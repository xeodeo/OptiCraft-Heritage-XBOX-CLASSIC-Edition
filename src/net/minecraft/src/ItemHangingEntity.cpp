#include "ItemHangingEntity.h"

#include "Direction.h"
#include "EntityHanging.h"
#include "EntityItemFrame.h"
#include "EntityPainting.h"
#include "EntityPlayer.h"
#include "ItemStack.h"
#include "World.h"

ItemHangingEntity::ItemHangingEntity(int id, HangingType type)
    : Item(id),
      hangingType(type)
{
}

bool ItemHangingEntity::onItemUse(ItemStack *stack, EntityPlayer *player, World *world, int x, int y, int z, int side)
{
    return onItemUse(stack, player, world, x, y, z, side, 0.0f, 0.0f, 0.0f);
}

bool ItemHangingEntity::onItemUse(ItemStack *stack, EntityPlayer *player, World *world, int x, int y, int z, int side, float, float, float)
{
    if (side == 0 || side == 1)
    {
        return false;
    }

    if (side < 0 || side >= 6)
    {
        return false;
    }

    int dir = Direction::facingToDirection[side];
    EntityHanging *entity = createHangingEntity(world, x, y, z, dir);

    if (player != nullptr && !player->canPlayerEdit(x, y, z, side, stack))
    {
        delete entity;
        return false;
    }

    if (entity != nullptr && entity->onValidSurface())
    {
        if (world != nullptr && !world->multiplayerWorld)
        {
            world->spawnEntityInWorld(entity);
        }

        if (stack != nullptr)
        {
            --stack->stackSize;
        }
        return true;
    }

    delete entity;
    return true;
}

EntityHanging *ItemHangingEntity::createHangingEntity(World *world, int x, int y, int z, int dir)
{
    if (hangingType == TYPE_PAINTING)
    {
        return new EntityPainting(world, x, y, z, dir);
    }
    else
    {
        return new EntityItemFrame(world, x, y, z, dir);
    }
}
