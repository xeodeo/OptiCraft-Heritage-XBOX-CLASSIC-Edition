#pragma once

#include "Container.h"

class TileEntityBeacon;
class InventoryPlayer;
class SlotBeacon;
class ICrafting;
class EntityPlayer;
class ItemStack;

class ContainerBeacon : public Container
{
public:
    ContainerBeacon(InventoryPlayer *playerInv, TileEntityBeacon *beacon);

    void addCraftingToCrafters(ICrafting *crafter) override;
    void updateProgressBar(int id, int value) override;
    bool isUsableByPlayer(EntityPlayer *player) override;
    bool canInteractWith(EntityPlayer *player) override { return isUsableByPlayer(player); }
    ItemStack *transferStackInSlot(EntityPlayer *player, int slotIndex) override;

    TileEntityBeacon *getBeacon() const { return theBeacon; }

private:
    TileEntityBeacon *theBeacon = nullptr;
    SlotBeacon *beaconSlot = nullptr;
    int field_82865_g = 0;
    int field_82867_h = 0;
    int field_82868_i = 0;
};
