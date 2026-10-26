#include "ItemHangingEntity.h"
#include "ItemStack.h"
#include "World.h"
#include "EntityPlayer.h"
bool ItemHangingEntity::onItemUse(ItemStack *stack, EntityPlayer *player, World *world, int x, int y, int z, int side)
{
    if (side < 2 || side > 5 || world->multiplayerWorld) return false;
    EntityItemFrame *frame = new EntityItemFrame(world, x, y, z, side);
    if (!frame->onValidSurface()) { delete frame; return false; }
    if (!world->entityJoinedWorld(frame)) { delete frame; return false; }
    --stack->stackSize;
    return true;
}
