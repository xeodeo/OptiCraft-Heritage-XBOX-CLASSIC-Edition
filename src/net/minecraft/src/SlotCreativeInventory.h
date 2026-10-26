#pragma once

#include "Slot.h"

class GuiContainerCreative;
class EntityPlayer;
class ItemStack;
class IInventory;

// net.minecraft.src.SlotCreativeInventory
class SlotCreativeInventory : public Slot
{
public:
    SlotCreativeInventory(GuiContainerCreative *container, Slot *slot, int slotIndex);

    void onPickupFromSlot(EntityPlayer *player, ItemStack *stack) override;
    bool isItemValid(ItemStack *stack) override;
    ItemStack *getStack() override;
    bool getHasStack() override;
    void putStack(ItemStack *stack) override;
    void onSlotChanged() override;
    int_t getSlotStackLimit() override;
    int_t getBackgroundIconIndex() override;
    ItemStack *decrStackSize(int_t amount) override;
    bool isSlotInInventory(IInventory *inv, int_t slot) override;

    static Slot *func_75240_a(SlotCreativeInventory *slot) { return slot ? slot->theSlot : nullptr; }

    Slot *theSlot;
    GuiContainerCreative *theCreativeInventory;
};
