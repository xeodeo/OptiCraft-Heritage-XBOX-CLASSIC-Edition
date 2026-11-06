#include "ContainerMerchant.h"

#include "EntityPlayer.h"
#include "IMerchant.h"
#include "InventoryMerchant.h"
#include "InventoryPlayer.h"
#include "ItemStack.h"
#include "Slot.h"
#include "SlotMerchantResult.h"
#include "World.h"

ContainerMerchant::ContainerMerchant(InventoryPlayer *inventory, IMerchant *merchant, World *world)
	: theMerchant(merchant), merchantInventory(new InventoryMerchant(inventory ? inventory->player : nullptr, merchant)), theWorld(world)
{
	addSlot(new Slot(merchantInventory, 0, 36, 53));
	addSlot(new Slot(merchantInventory, 1, 62, 53));
	addSlot(new SlotMerchantResult(inventory ? inventory->player : nullptr, merchant, merchantInventory, 2, 120, 53));
	if (inventory)
	{
		for (int row = 0; row < 3; ++row)
			for (int col = 0; col < 9; ++col)
				addSlot(new Slot(inventory, col + row * 9 + 9, 8 + col * 18, 84 + row * 18));
		for (int col = 0; col < 9; ++col) addSlot(new Slot(inventory, col, 8 + col * 18, 142));
	}
}
ContainerMerchant::~ContainerMerchant() { delete merchantInventory; }
InventoryMerchant *ContainerMerchant::getMerchantInventory() const { return merchantInventory; }
void ContainerMerchant::setCurrentRecipeIndex(int_t index) { merchantInventory->setCurrentRecipeIndex(index); }
void ContainerMerchant::onCraftMatrixChanged(IInventory *inventory) { (void)inventory; merchantInventory->resetRecipeAndSlots(); Container::onCraftMatrixChanged(merchantInventory); }
void ContainerMerchant::detachMerchant()
{
	theMerchant = nullptr;
	merchantInventory->detachMerchant();
	if (slots.size() > 2)
		if (auto *result = dynamic_cast<SlotMerchantResult *>(slots[2])) result->detachMerchant();
}
bool ContainerMerchant::isUsableByPlayer(EntityPlayer *player) { return theMerchant && theMerchant->getCustomer() == player; }

ItemStack *ContainerMerchant::getStackInSlot(int_t slotIndex)
{
	if (slotIndex < 0 || slotIndex >= static_cast<int_t>(slots.size()))
		return nullptr;

	Slot *slot = slots[slotIndex];
	if (slot == nullptr || !slot->getHasStack())
		return nullptr;

	ItemStack *source = slot->getStack();
	ItemStack *original = source->copy();
	bool moved = false;

	if (slotIndex == 2)
	{
		moved = mergeItemStack(source, 3, 39, true);
		if (moved)
			slot->onSlotChange(source, original);
	}
	else if (slotIndex != 0 && slotIndex != 1)
	{
		if (slotIndex >= 3 && slotIndex < 30)
			moved = mergeItemStack(source, 30, 39, false);
		else if (slotIndex >= 30 && slotIndex < 39)
			moved = mergeItemStack(source, 3, 30, false);
	}
	else
	{
		moved = mergeItemStack(source, 3, 39, false);
	}

	if (!moved)
	{
		delete original;
		return nullptr;
	}

	ItemStack *orphaned = nullptr;
	if (source->stackSize == 0)
	{
		orphaned = slot->takeStack();
	}
	else
	{
		slot->onSlotChanged();
	}

	if (source->stackSize == original->stackSize)
	{
		delete orphaned;
		delete original;
		return nullptr;
	}

	slot->onPickupFromSlot(source);
	delete orphaned;
	return original;
}

void ContainerMerchant::onCraftGuiClosed(EntityPlayer *player)
{
	Container::onCraftGuiClosed(player);
	if (theMerchant) theMerchant->setCustomer(nullptr);
	if (player && theWorld && !theWorld->multiplayerWorld)
		for (int slot = 0; slot < 2; ++slot)
			if (ItemStack *stack = merchantInventory->getStackInSlotOnClosing(slot)) player->dropPlayerItem(stack);
}
