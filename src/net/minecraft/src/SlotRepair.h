#pragma once
#include "Slot.h"
class ContainerRepair;
class SlotRepair final : public Slot {
public:
    SlotRepair(ContainerRepair *container, IInventory *inventory, int index, int x, int y);
    void onPickupFromSlot(ItemStack *stack) override;
private:
    ContainerRepair *container;
};
