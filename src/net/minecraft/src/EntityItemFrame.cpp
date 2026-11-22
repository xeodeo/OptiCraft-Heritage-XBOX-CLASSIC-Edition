#include "EntityItemFrame.h"

#include "DataWatcher.h"
#include "EntityPlayer.h"
#include "InventoryPlayer.h"
#include "Item.h"
#include "ItemStack.h"
#include "ItemMap.h"
#include "MapData.h"
#include "NBTTagCompound.h"
#include "World.h"
#include "java/Random.h"

EntityItemFrame::EntityItemFrame(World *world)
    : EntityHanging(world)
{
    ensureEntityInit();
}

EntityItemFrame::EntityItemFrame(World *world, int x, int y, int z, int dir)
    : EntityHanging(world, x, y, z, dir)
{
    ensureEntityInit();
    setDirection(dir);
}

void EntityItemFrame::entityInit()
{
    dataWatcher->addObjectByDataType(2, 5);
    dataWatcher->addObject(3, static_cast<byte_t>(0));
}

int EntityItemFrame::func_82329_d()
{
    return 9;
}

int EntityItemFrame::func_82330_g()
{
    return 9;
}

bool EntityItemFrame::isInRangeToRenderDist(double dist)
{
    double maxDist = 16.0 * 64.0 * renderDistanceWeight;
    return dist < maxDist * maxDist;
}

void EntityItemFrame::dropItemStack()
{
    if (Item::itemFrame != nullptr)
    {
        entityDropItem(new ItemStack(Item::itemFrame), 0.0f);
    }

    ItemStack *stack = getDisplayedItem();
    if (stack != nullptr && rand.nextFloat() < itemDropChance)
    {
        ItemStack *drop = stack->copy();
        drop->setItemFrame(nullptr);
        entityDropItem(drop, 0.0f);
    }
}

ItemStack *EntityItemFrame::getDisplayedItem()
{
    return dataWatcher->getWatchableObjectItemStack(2);
}

void EntityItemFrame::setDisplayedItem(ItemStack *stack)
{
    if (stack != nullptr)
    {
        ItemStack *copy = stack->copy();
        copy->stackSize = 1;
        copy->setItemFrame(this);
        dataWatcher->updateObject(2, copy);
        dataWatcher->setObjectWatched(2);
    }
    else
    {
        dataWatcher->updateObject(2, static_cast<ItemStack *>(nullptr));
        dataWatcher->setObjectWatched(2);
    }
}

int EntityItemFrame::getRotation()
{
    return static_cast<int>(dataWatcher->getWatchableObjectByte(3));
}

void EntityItemFrame::setItemRotation(int rot)
{
    dataWatcher->updateObject(3, static_cast<byte_t>(rot % 4));
}

void EntityItemFrame::writeEntityToNBT(NBTTagCompound *nbt)
{
    if (nbt == nullptr)
        return;

    ItemStack *stack = getDisplayedItem();
    if (stack != nullptr)
    {
        NBTTagCompound *itemTag = new NBTTagCompound();
        stack->writeToNBT(itemTag);
        nbt->setCompoundTag("Item", itemTag);
        nbt->setByte("ItemRotation", static_cast<byte_t>(getRotation()));
        nbt->setFloat("ItemDropChance", itemDropChance);
    }

    EntityHanging::writeEntityToNBT(nbt);
}

void EntityItemFrame::readEntityFromNBT(NBTTagCompound *nbt)
{
    if (nbt == nullptr)
        return;

    NBTTagCompound *itemTag = nbt->getCompoundTag("Item");
    if (itemTag != nullptr && !itemTag->hasNoTags())
    {
        ItemStack *loaded = ItemStack::loadItemStackFromNBT(itemTag);
        setDisplayedItem(loaded);
        setItemRotation(nbt->getByte("ItemRotation"));
        if (nbt->hasKey("ItemDropChance"))
        {
            itemDropChance = nbt->getFloat("ItemDropChance");
        }
    }

    EntityHanging::readEntityFromNBT(nbt);
}

bool EntityItemFrame::interact(EntityPlayer *player)
{
    if (player == nullptr)
        return false;

    if (getDisplayedItem() == nullptr)
    {
        ItemStack *held = player->getHeldItem();
        if (held != nullptr && worldObj != nullptr && !worldObj->multiplayerWorld)
        {
            setDisplayedItem(held);
            if (!player->capabilities.isCreativeMode)
            {
                if (--held->stackSize <= 0)
                {
                    player->inventory->setInventorySlotContents(player->inventory->currentItem, nullptr);
                }
            }
        }
    }
    else if (worldObj != nullptr && !worldObj->multiplayerWorld)
    {
        setItemRotation(getRotation() + 1);
    }

    return true;
}

void EntityItemFrame::onUpdate()
{
    EntityHanging::onUpdate();

    // Java 1.5.2 does this in EntityTrackerEntry (every 10 ticks, for each player
    // tracking the frame, range 160): a map in a frame refreshes its MapData so
    // the green frame marker shows. There is no server tracker here.
    const int tick = mapTrackerTicks++;
    if (isDead || worldObj == nullptr || worldObj->multiplayerWorld || tick % 10 != 0)
        return;
    ItemStack *stack = getDisplayedItem();
    if (stack == nullptr || Item::mapItem == nullptr || stack->itemID != Item::mapItem->shiftedIndex)
        return;
    MapData *mapData = static_cast<ItemMap *>(Item::mapItem)->getMapData(stack, worldObj);
    if (mapData == nullptr)
        return;
    for (EntityPlayer *player : worldObj->playerEntities)
    {
        if (player == nullptr)
            continue;
        const double dx = player->posX - posX;
        const double dz = player->posZ - posZ;
        if (dx >= -160.0 && dx <= 160.0 && dz >= -160.0 && dz <= 160.0)
            mapData->updateVisiblePlayers(player, stack);
    }
}
