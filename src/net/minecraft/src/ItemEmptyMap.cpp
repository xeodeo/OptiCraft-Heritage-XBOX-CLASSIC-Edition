#include "ItemEmptyMap.h"
#include "ItemStack.h"
#include "Item.h"
#include "World.h"
#include "EntityPlayer.h"
#include "InventoryPlayer.h"
#include "MapData.h"
#include "WorldProvider.h"
#include "MathHelper.h"
#include <cmath>

ItemEmptyMap::ItemEmptyMap(int id)
    : ItemMapBase(id)
{
}

ItemStack *ItemEmptyMap::onItemRightClick(ItemStack *itemstack, World *world, EntityPlayer *entityplayer)
{
    if (world == nullptr || entityplayer == nullptr || itemstack == nullptr)
    {
        return itemstack;
    }

    ItemStack *mapStack = new ItemStack(Item::mapItem, 1, world->getUniqueDataId("map"));
    std::string s = "map_" + std::to_string(mapStack->getItemDamage());
    MapData *mapData = new MapData(s);
    world->setItemData(s, mapData);

    mapData->scale = 0;
    int scaleFactor = 128 * (1 << mapData->scale);
    mapData->xCenter = (int)(std::round(entityplayer->posX / (double)scaleFactor) * (long)scaleFactor);
    mapData->zCenter = (int)(std::round(entityplayer->posZ / (double)scaleFactor) * (long)scaleFactor);
    if (world->worldProvider != nullptr)
    {
        mapData->dimension = (byte_t)world->worldProvider->worldType;
    }
    mapData->markDirty();

    itemstack->stackSize--;
    if (itemstack->stackSize <= 0)
    {
        return mapStack;
    }

    // addItemStackToInventory moves the items out of the stack it is given.
    if (!entityplayer->inventory->addItemStackToInventory(mapStack))
    {
        entityplayer->dropPlayerItem(mapStack);
    }
    else
    {
        delete mapStack;
    }

    return itemstack;
}
