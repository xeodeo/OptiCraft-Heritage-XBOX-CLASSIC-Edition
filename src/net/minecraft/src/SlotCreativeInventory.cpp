#include "SlotCreativeInventory.h"

SlotCreativeInventory::SlotCreativeInventory(GuiContainerCreative *container, Slot *slot, int slotIndex)
    : Slot(slot != nullptr ? slot->getInventory() : nullptr, slotIndex, 0, 0)
    , theSlot(slot)
    , theCreativeInventory(container)
{
}

void SlotCreativeInventory::onPickupFromSlot(EntityPlayer *player, ItemStack *stack)
{
    if (theSlot != nullptr)
        theSlot->onPickupFromSlot(player, stack);
}

bool SlotCreativeInventory::isItemValid(ItemStack *stack)
{
    return theSlot != nullptr ? theSlot->isItemValid(stack) : true;
}

ItemStack *SlotCreativeInventory::getStack()
{
    return theSlot != nullptr ? theSlot->getStack() : nullptr;
}

bool SlotCreativeInventory::getHasStack()
{
    return theSlot != nullptr ? theSlot->getHasStack() : false;
}

void SlotCreativeInventory::putStack(ItemStack *stack)
{
    if (theSlot != nullptr)
        theSlot->putStack(stack);
}

void SlotCreativeInventory::onSlotChanged()
{
    if (theSlot != nullptr)
        theSlot->onSlotChanged();
}

int_t SlotCreativeInventory::getSlotStackLimit()
{
    return theSlot != nullptr ? theSlot->getSlotStackLimit() : 64;
}

int_t SlotCreativeInventory::getBackgroundIconIndex()
{
    return theSlot != nullptr ? theSlot->getBackgroundIconIndex() : -1;
}

ItemStack *SlotCreativeInventory::decrStackSize(int_t amount)
{
    return theSlot != nullptr ? theSlot->decrStackSize(amount) : nullptr;
}

bool SlotCreativeInventory::isSlotInInventory(IInventory *inv, int_t slot)
{
    return theSlot != nullptr ? theSlot->isSlotInInventory(inv, slot) : false;
}
