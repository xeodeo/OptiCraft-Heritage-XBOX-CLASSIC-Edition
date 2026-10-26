#pragma once

#include "Slot.h"

class EntityPlayer;
class IMerchant;
class InventoryMerchant;

class SlotMerchantResult : public Slot
{
public:
	SlotMerchantResult(EntityPlayer *player, IMerchant *merchant, InventoryMerchant *inventory, int_t index, int_t x, int_t y);
	bool isItemValid(ItemStack *stack) override;
	ItemStack *decrStackSize(int_t amount) override;
	void onPickupFromSlot(ItemStack *stack) override;

private:
	EntityPlayer *thePlayer;
	IMerchant *theMerchant;
	InventoryMerchant *merchantInventory;
	int_t removeCount = 0;
};
