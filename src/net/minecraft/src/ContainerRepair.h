#pragma once

#include <string>
#include "Container.h"
#include "IInventory.h"

class EntityPlayer;
class InventoryPlayer;
class ICrafting;
class ItemStack;
class World;

class ContainerRepair : public Container
{
public:
    ContainerRepair(InventoryPlayer *playerInv, World *world, int x, int y, int z, EntityPlayer *player);
    ~ContainerRepair() override;

    void onCraftMatrixChanged(IInventory *inv) override;
    void updateRepairOutput();
    void addCraftingToCrafters(ICrafting *crafter) override;
    void updateProgressBar(int id, int value) override;
    void onCraftGuiClosed(EntityPlayer *player) override;
    bool isUsableByPlayer(EntityPlayer *player) override;
    bool canInteractWith(EntityPlayer *player) override { return isUsableByPlayer(player); }
    ItemStack *transferStackInSlot(EntityPlayer *player, int slotIndex) override;
    void updateItemName(const std::string &name);

    IInventory *getInputSlots() const { return inputSlots; }
    int getStackSizeToBeUsedInRepair() const { return stackSizeToBeUsedInRepair; }

    int maximumCost = 0;

private:
    IInventory *outputSlot = nullptr;
    IInventory *inputSlots = nullptr;
    World *theWorld = nullptr;
    int field_82861_i = 0;
    int field_82858_j = 0;
    int field_82859_k = 0;
    int stackSizeToBeUsedInRepair = 0;
    std::string repairedItemName;
    EntityPlayer *thePlayer = nullptr;
};
