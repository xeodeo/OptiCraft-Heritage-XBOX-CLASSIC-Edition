#include "ContainerCreative.h"

#include "EntityPlayer.h"
#include "GuiContainerCreative.h"
#include "InventoryBasic.h"
#include "InventoryPlayer.h"
#include "ItemStack.h"
#include "Slot.h"

ContainerCreative::ContainerCreative(EntityPlayer *player)
{
    InventoryPlayer *playerInventory = player->inventory;

    for (int_t row = 0; row < 5; ++row)
    {
        for (int_t col = 0; col < 9; ++col)
        {
            addSlot(new Slot(GuiContainerCreative::getInventory(), row * 9 + col, 9 + col * 18, 18 + row * 18));
        }
    }

    for (int_t col = 0; col < 9; ++col)
    {
        addSlot(new Slot(playerInventory, col, 9 + col * 18, 112));
    }

    scrollTo(0.0f);
}

ContainerCreative::~ContainerCreative()
{
    clearItemList();
}

void ContainerCreative::clearItemList()
{
    InventoryBasic *inv = GuiContainerCreative::getInventory();
    if (inv != nullptr)
    {
        for (int_t i = 0; i < 45; ++i)
        {
            inv->setInventorySlotContents(i, nullptr);
        }
    }
    for (ItemStack *stack : itemList)
    {
        delete stack;
    }
    itemList.clear();
}

bool ContainerCreative::isUsableByPlayer(EntityPlayer *player)
{
    (void)player;
    return true;
}

bool ContainerCreative::canInteractWith(EntityPlayer *player)
{
    (void)player;
    return true;
}

void ContainerCreative::scrollTo(float_t scroll)
{
    const int_t rowCount = (int_t)itemList.size() / 9 - 5 + 1;
    int_t targetRow = (int_t)((double)(scroll * (float_t)rowCount) + 0.5);
    if (targetRow < 0)
        targetRow = 0;

    InventoryBasic *inv = GuiContainerCreative::getInventory();
    for (int_t row = 0; row < 5; ++row)
    {
        for (int_t col = 0; col < 9; ++col)
        {
            const int_t itemIndex = col + (row + targetRow) * 9;
            const int_t slotIndex = col + row * 9;
            if (itemIndex >= 0 && itemIndex < (int_t)itemList.size())
            {
                inv->setInventorySlotContents(slotIndex, itemList[itemIndex]);
            }
            else
            {
                inv->setInventorySlotContents(slotIndex, nullptr);
            }
        }
    }
}

bool ContainerCreative::hasMoreThan1PageOfItemsInList() const
{
    return itemList.size() > 45;
}

void ContainerCreative::retrySlotClick(int_t slot, int_t button, bool shift, EntityPlayer *player)
{
    (void)slot;
    (void)button;
    (void)shift;
    (void)player;
}

ItemStack *ContainerCreative::transferStackInSlot(EntityPlayer *player, int_t slotIndex)
{
    (void)player;
    if (slotIndex >= (int_t)slots.size() - 9 && slotIndex < (int_t)slots.size())
    {
        Slot *slot = slots[slotIndex];
        if (slot != nullptr && slot->getHasStack())
        {
            slot->putStack(nullptr);
        }
    }
    return nullptr;
}

bool ContainerCreative::func_94530_a(ItemStack *stack, Slot *slot)
{
    (void)stack;
    return slot != nullptr && slot->yDisplayPosition > 90;
}

bool ContainerCreative::func_94531_b(Slot *slot)
{
    if (slot == nullptr)
        return false;
    return dynamic_cast<InventoryPlayer *>(slot->getInventory()) != nullptr ||
           (slot->yDisplayPosition > 90 && slot->xDisplayPosition <= 162);
}
