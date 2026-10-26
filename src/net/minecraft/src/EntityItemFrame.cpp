#include "EntityItemFrame.h"
#include "DataWatcher.h"
#include "EntityPlayer.h"
#include "InventoryPlayer.h"
#include "Item.h"
#include "ItemStack.h"
#include "NBTTagCompound.h"
#include "World.h"

EntityItemFrame::EntityItemFrame(World *world) : EntityPainting(world) { ensureEntityInit(); }
EntityItemFrame::EntityItemFrame(World *world, int_t x, int_t y, int_t z, int_t dir) : EntityPainting(world, x, y, z, dir) { ensureEntityInit(); setDirection(dir); }
EntityItemFrame::~EntityItemFrame() = default;
void EntityItemFrame::entityInit() {
    if (!getDataWatcher()->hasObject(2)) getDataWatcher()->addObject(2, static_cast<ItemStack *>(nullptr));
    if (!getDataWatcher()->hasObject(3)) getDataWatcher()->addObject(3, static_cast<byte_t>(0));
}
ItemStack *EntityItemFrame::getDisplayedItem() const { return const_cast<EntityItemFrame *>(this)->getDataWatcher()->getWatchableObjectItemStack(2); }
void EntityItemFrame::setDisplayedItem(ItemStack *stack) {
    ItemStack *copy = stack == nullptr ? nullptr : stack->copy();
    if (copy != nullptr) copy->stackSize = 1;
    getDataWatcher()->updateObject(2, copy);
}
int_t EntityItemFrame::getRotation() const { return const_cast<EntityItemFrame *>(this)->getDataWatcher()->getWatchableObjectByte(3); }
void EntityItemFrame::setItemRotation(int_t rotation) { getDataWatcher()->updateObject(3, static_cast<byte_t>(rotation & 3)); }
void EntityItemFrame::dropItemStack() {
    if (worldObj->multiplayerWorld) return;
    entityDropItem(new ItemStack(Item::itemFrame), 0.0f);
    if (getDisplayedItem() != nullptr) entityDropItem(getDisplayedItem()->copy(), 0.0f);
    setDisplayedItem(nullptr);
}
bool EntityItemFrame::interact(EntityPlayer *player) {
    if (getDisplayedItem() == nullptr) {
        ItemStack *held = player->inventory->getCurrentItem();
        if (held != nullptr && !worldObj->multiplayerWorld) {
            setDisplayedItem(held);
            if (!player->capabilities.isCreativeMode) {
                --held->stackSize;
                if (held->stackSize <= 0) player->inventory->setInventorySlotContents(player->inventory->currentItem, nullptr);
            }
        }
    } else if (!worldObj->multiplayerWorld) setItemRotation(getRotation() + 1);
    return true;
}
bool EntityItemFrame::attackEntityFrom(Entity *entity, int_t damage) { if (!isDead && !worldObj->multiplayerWorld) { setEntityDead(); dropItemStack(); } return true; }
void EntityItemFrame::writeEntityToNBT(NBTTagCompound *tag) {
    EntityPainting::writeEntityToNBT(tag);
    if (getDisplayedItem() != nullptr) { tag->setCompoundTag("Item", getDisplayedItem()->writeToNBT(new NBTTagCompound())); tag->setByte("ItemRotation", static_cast<byte_t>(getRotation())); tag->setFloat("ItemDropChance", itemDropChance); }
}
void EntityItemFrame::readEntityFromNBT(NBTTagCompound *tag) {
    NBTTagCompound *item = tag->getCompoundTag("Item");
    if (item != nullptr && item->getTags().size() > 0) { setDisplayedItem(ItemStack::loadItemStackFromNBT(item)); setItemRotation(tag->getByte("ItemRotation")); if (tag->hasKey("ItemDropChance")) itemDropChance = tag->getFloat("ItemDropChance"); }
    EntityPainting::readEntityFromNBT(tag);
}
