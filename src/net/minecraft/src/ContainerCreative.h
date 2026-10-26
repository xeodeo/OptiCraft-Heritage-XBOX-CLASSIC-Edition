#pragma once

#include "Container.h"
#include <vector>

class EntityPlayer;
class ItemStack;
class Slot;

// net.minecraft.src.ContainerCreative
class ContainerCreative : public Container
{
public:
    explicit ContainerCreative(EntityPlayer *player);
    ~ContainerCreative() override;

    bool canInteractWith(EntityPlayer *player) override;
    void scrollTo(float_t scroll);
    bool hasMoreThan1PageOfItemsInList() const;

    ItemStack *transferStackInSlot(EntityPlayer *player, int_t slotIndex) override;
    bool func_94530_a(ItemStack *stack, Slot *slot);
    bool func_94531_b(Slot *slot);

    void clearItemList();

protected:
    void retrySlotClick(int_t slot, int_t button, bool shift, EntityPlayer *player) override;

public:
    std::vector<ItemStack *> itemList;
};
