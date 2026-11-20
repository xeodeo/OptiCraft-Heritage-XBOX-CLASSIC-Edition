#include "ContainerBeacon.h"

#include "EntityPlayer.h"
#include "ICrafting.h"
#include "InventoryPlayer.h"
#include "ItemStack.h"
#include "Slot.h"
#include "SlotBeacon.h"
#include "TileEntityBeacon.h"
#include "World.h"

ContainerBeacon::ContainerBeacon(InventoryPlayer *playerInv, TileEntityBeacon *beacon)
    : theBeacon(beacon)
{
    beaconSlot = new SlotBeacon(this, beacon, 0, 136, 110);
    addSlot(beaconSlot);

    constexpr int offsetX = 36;
    constexpr int offsetY = 137;

    for (int_t row = 0; row < 3; ++row)
    {
        for (int_t col = 0; col < 9; ++col)
        {
            addSlot(new Slot(playerInv, col + row * 9 + 9, offsetX + col * 18, offsetY + row * 18));
        }
    }

    for (int_t col = 0; col < 9; ++col)
    {
        addSlot(new Slot(playerInv, col, offsetX + col * 18, 58 + offsetY));
    }

    if (beacon != nullptr)
    {
        field_82865_g = beacon->getLevels();
        field_82867_h = beacon->getPrimaryEffect();
        field_82868_i = beacon->getSecondaryEffect();
    }
}

void ContainerBeacon::addCraftingToCrafters(ICrafting *crafter)
{
    Container::addCraftingToCrafters(crafter);
    if (crafter != nullptr)
    {
        crafter->sendProgressBarUpdate(this, 0, field_82865_g);
        crafter->sendProgressBarUpdate(this, 1, field_82867_h);
        crafter->sendProgressBarUpdate(this, 2, field_82868_i);
    }
}

void ContainerBeacon::updateProgressBar(int id, int value)
{
    if (theBeacon == nullptr)
        return;

    if (id == 0)
    {
        theBeacon->setLevels(value);
    }
    else if (id == 1)
    {
        theBeacon->setPrimaryEffect(value);
    }
    else if (id == 2)
    {
        theBeacon->setSecondaryEffect(value);
    }
}

bool ContainerBeacon::isUsableByPlayer(EntityPlayer *player)
{
    return theBeacon != nullptr && theBeacon->canInteractWith(player);
}

void ContainerBeacon::onCraftGuiClosed(EntityPlayer *player)
{
    Container::onCraftGuiClosed(player);
    if (theBeacon == nullptr || theBeacon->worldObj == nullptr || theBeacon->worldObj->multiplayerWorld || player == nullptr)
        return;

    ItemStack *stack = theBeacon->getStackInSlotOnClosing(0);
    if (stack != nullptr)
    {
        if (player->inventory != nullptr && player->inventory->addItemStackToInventory(stack))
        {
            if (stack->stackSize <= 0)
                delete stack;
            else
                player->dropPlayerItem(stack);
        }
        else
        {
            player->dropPlayerItem(stack);
        }
    }
}

bool ContainerBeacon::confirmSelection(int_t primaryEffectId, int_t secondaryEffectId)
{
    // Mirrors Java 1.5.2 NetServerHandler.handleCustomPayload("MC|Beacon"):
    // verify the payment slot holds a valid payment item before consuming 1
    // unit and committing the staged effects onto the live TileEntityBeacon.
    if (theBeacon == nullptr || beaconSlot == nullptr || !beaconSlot->getHasStack())
        return false;

    ItemStack *payStack = beaconSlot->getStack();
    if (payStack == nullptr || payStack->stackSize <= 0 || !beaconSlot->isItemValid(payStack))
        return false;

    if (!theBeacon->isPrimaryEffectAllowed(primaryEffectId))
        return false;

    delete beaconSlot->decrStackSize(1);
    beaconSlot->onSlotChanged();
    theBeacon->setPrimaryEffect(primaryEffectId);
    theBeacon->setSecondaryEffect(secondaryEffectId);
    theBeacon->onInventoryChanged();
    return true;
}

ItemStack *ContainerBeacon::transferStackInSlot(EntityPlayer *player, int slotIndex)
{
    ItemStack *result = nullptr;
    Slot *slot = getSlot(slotIndex);
    if (slot != nullptr && slot->getHasStack())
    {
        ItemStack *stack = slot->getStack();
        result = stack->copy();
        if (slotIndex == 0)
        {
            if (!mergeItemStack(stack, 1, 37, true))
            {
                delete result;
                return nullptr;
            }
            slot->onSlotChange(stack, result);
        }
        else if (beaconSlot != nullptr && !beaconSlot->getHasStack() && beaconSlot->isItemValid(stack) && stack->stackSize == 1)
        {
            if (!mergeItemStack(stack, 0, 1, false))
            {
                delete result;
                return nullptr;
            }
        }
        else if (slotIndex >= 1 && slotIndex < 28)
        {
            if (!mergeItemStack(stack, 28, 37, false))
            {
                delete result;
                return nullptr;
            }
        }
        else if (slotIndex >= 28 && slotIndex < 37)
        {
            if (!mergeItemStack(stack, 1, 28, false))
            {
                delete result;
                return nullptr;
            }
        }
        else if (!mergeItemStack(stack, 1, 37, false))
        {
            delete result;
            return nullptr;
        }

        // putStack(nullptr) deletes the stack: keep its size, and give
        // onPickupFromSlot the copy (Java passes the now empty stack).
        const int remaining = stack->stackSize;
        if (remaining == 0)
        {
            slot->putStack(nullptr);
        }
        else
        {
            slot->onSlotChanged();
        }

        if (remaining == result->stackSize)
        {
            delete result;
            return nullptr;
        }

        slot->onPickupFromSlot(player, remaining == 0 ? result : stack);
    }
    return result;
}
