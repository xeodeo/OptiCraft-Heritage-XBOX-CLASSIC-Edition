#include "SlotMerchantResult.h"

#include <algorithm>
#include "EntityPlayer.h"
#include "IMerchant.h"
#include "InventoryMerchant.h"
#include "ItemStack.h"
#include "MerchantRecipe.h"

SlotMerchantResult::SlotMerchantResult(EntityPlayer *player, IMerchant *merchant, InventoryMerchant *inventory, int_t index, int_t x, int_t y)
	: Slot(inventory, index, x, y), thePlayer(player), theMerchant(merchant), merchantInventory(inventory) {}
bool SlotMerchantResult::isItemValid(ItemStack *) { return false; }
ItemStack *SlotMerchantResult::decrStackSize(int_t amount)
{
	if (getHasStack()) removeCount += std::min(amount, getStack()->stackSize);
	return Slot::decrStackSize(amount);
}
void SlotMerchantResult::onPickupFromSlot(ItemStack *stack)
{
	if (stack && thePlayer) stack->onCrafting(thePlayer->worldObj, thePlayer, removeCount);
	removeCount = 0;
	MerchantRecipe *recipe = merchantInventory ? merchantInventory->getCurrentRecipe() : nullptr;
	if (!recipe) return;
	ItemStack *first = merchantInventory->getStackInSlot(0), *second = merchantInventory->getStackInSlot(1);
	auto consume = [&](ItemStack *buy, ItemStack *other)
	{
		ItemStack *needA = recipe->getItemToBuy(), *needB = recipe->getSecondItemToBuy();
		if (!buy || !needA || buy->itemID != needA->itemID) return false;
		if (needB) { if (!other || other->itemID != needB->itemID) return false; }
		else if (other) return false;
		buy->stackSize -= needA->stackSize;
		if (needB) other->stackSize -= needB->stackSize;
		return true;
	};
	if (!consume(first, second) && !consume(second, first)) return;
	if (first && first->stackSize <= 0) first = nullptr;
	if (second && second->stackSize <= 0) second = nullptr;
	merchantInventory->setInventorySlotContents(0, first);
	merchantInventory->setInventorySlotContents(1, second);
	if (theMerchant) theMerchant->useRecipe(recipe);
}
