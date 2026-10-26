#pragma once

#include "Slot.h"

class ContainerRepair;
class IInventory;
class World;
class EntityPlayer;
class ItemStack;

class SlotRepair : public Slot
{
public:
    SlotRepair(ContainerRepair *container, IInventory *inv, int index, int x, int y, World *world, int blockX, int blockY, int blockZ);

    bool isItemValid(ItemStack *stack) override;
    bool canTakeStack(EntityPlayer *player) override;
    void onPickupFromSlot(ItemStack *stack) override { onPickupFromSlot(nullptr, stack); }
    void onPickupFromSlot(EntityPlayer *player, ItemStack *stack) override;

private:
    ContainerRepair *anvil = nullptr;
    World *theWorld = nullptr;
    int blockPosX = 0;
    int blockPosY = 0;
    int blockPosZ = 0;
};
