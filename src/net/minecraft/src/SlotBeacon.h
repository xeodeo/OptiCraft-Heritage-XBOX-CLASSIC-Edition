#pragma once

#include "Slot.h"

class ContainerBeacon;
class IInventory;
class ItemStack;

class SlotBeacon : public Slot
{
public:
    SlotBeacon(ContainerBeacon *container, IInventory *inv, int index, int x, int y);

    bool isItemValid(ItemStack *stack) override;
    int getSlotStackLimit() override;

private:
    ContainerBeacon *beacon = nullptr;
};
