#include "SlotCreativeInventory.h"

SlotCreativeInventory::SlotCreativeInventory(GuiContainerCreative *container, Slot *slot, int slotIndex)
    : Slot(slot->inventory, slotIndex, 0, 0)
    , theSlot(slot)
    , theCreativeInventory(container)
{
}

void SlotCreativeInventory::onPickupFromSlot(EntityPlayer *player, ItemStack *stack)
{
    theSlot->onPickupFromSlot(player, stack);
}

bool SlotCreativeInventory::isItemValid(ItemStack *stack)
{
    return theSlot->isItemValid(stack);
}

ItemStack *SlotCreativeInventory::getStack()
{
    return theSlot->getStack();
}

bool SlotCreativeInventory::getHasStack()
{
    return theSlot->getHasStack();
}

void SlotCreativeInventory::putStack(ItemStack *stack)
{
    theSlot->putStack(stack);
}

void SlotCreativeInventory::onSlotChanged()
{
    theSlot->onSlotChanged();
}

int_t SlotCreativeInventory::getSlotStackLimit()
{
    return theSlot->getSlotStackLimit();
}

int_t SlotCreativeInventory::getBackgroundIconIndex()
{
    return theSlot->getBackgroundIconIndex();
}

ItemStack *SlotCreativeInventory::decrStackSize(int_t amount)
{
    return theSlot->decrStackSize(amount);
}

bool SlotCreativeInventory::isSlotInInventory(IInventory *inv, int_t slot)
{
    return theSlot->isSlotInInventory(inv, slot);
}
