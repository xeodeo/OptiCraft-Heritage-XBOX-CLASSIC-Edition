#pragma once
#include "Slot.h"
class ContainerRepair;
class SlotRepair final : public Slot {
public:
    SlotRepair(ContainerRepair *container, IInventory *inventory, int index, int x, int y);
    void onPickupFromSlot(ItemStack *stack) override;
    bool isItemValid(ItemStack *stack) override;
    bool isOutputSlot() const;
    bool hasResult() const;
    int requiredLevels() const;
    ContainerRepair *getRepairContainer() const;
    void consumeResult(ItemStack *stack);
    bool canAccept(ItemStack *stack) const;
    bool isEmpty() const;
    bool isReady() const;
    int cost() const;
    void pickup();
    ItemStack *result() const;
private:
    ContainerRepair *container;
};
