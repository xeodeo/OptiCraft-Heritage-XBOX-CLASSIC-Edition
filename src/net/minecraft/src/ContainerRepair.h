#pragma once
#include "Container.h"
#include "IInventory.h"
#include "IInvBasic.h"
#include "InventoryBasic.h"
#include "InventoryCraftResult.h"
class InventoryPlayer;
class World;
class ContainerRepair : public Container, public IInvBasic {
public:
    ContainerRepair(InventoryPlayer *player, World *world, int x, int y, int z, EntityPlayer *owner);
    ~ContainerRepair() override;
    bool isUsableByPlayer(EntityPlayer *player) override;
    void onCraftMatrixChanged(IInventory *inventory) override;
    void onInventoryChanged(IInventory *inventory) override;
    void updateRepairOutput();
    void takeOutput(EntityPlayer *player);
    void updateItemName(const std::string &name);
    ItemStack *getStackInSlot(int_t index) override;
    IInventory *getRepairInventory() const { return input; }
    int maximumCost = 0;
private:
    InventoryBasic *input;
    InventoryCraftResult *output;
    InventoryPlayer *playerInventory;
    World *world;
    EntityPlayer *owner;
    int x, y, z;
    std::string repairedItemName;
};
