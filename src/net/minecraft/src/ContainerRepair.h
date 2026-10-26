#pragma once
#include "Container.h"
#include "IInventory.h"
class InventoryPlayer;
class World;
class ContainerRepair : public Container {
public:
    ContainerRepair(InventoryPlayer *player, World *world, int x, int y, int z);
    ~ContainerRepair() override;
    bool isUsableByPlayer(EntityPlayer *player) override;
    void onCraftMatrixChanged(IInventory *inventory) override;
    ItemStack *getStackInSlot(int_t index) override;
    IInventory *getRepairInventory() const { return repair; }
private:
    IInventory *repair;
    InventoryPlayer *playerInventory;
};
