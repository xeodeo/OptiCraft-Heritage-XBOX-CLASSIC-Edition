#include "Container.h"

#include "Slot.h"
#include "ItemStack.h"
#include "ICrafting.h"
#include "EntityPlayer.h"
#include "InventoryPlayer.h"
#include "IInventory.h"
#include "MathHelper.h"
#include <algorithm>

Container::Container()
{
	inventoryItemStacks.clear();
	slots.clear();
	windowId = 0;
	transactionID = 0;
	crafters.clear();
	playerList.clear();
}

Container::~Container()
{
	for (Slot *slot : slots)
		delete slot;
	for (ItemStack *stack : inventoryItemStacks)
		delete stack;
}

void Container::addSlot(Slot *slot)
{
	slot->slotNumber = slots.size();
	slots.push_back(slot);
	inventoryItemStacks.push_back(nullptr);
}

void Container::updateCraftingResults()
{
	for(size_t i = 0; i < slots.size(); i++)
	{
		ItemStack *itemstack = slots[i]->getStack();
		ItemStack *itemstack1 = inventoryItemStacks[i];
		if(ItemStack::areItemStacksEqual(itemstack1, itemstack))
		{
			continue;
		}
		delete itemstack1;
		itemstack1 = itemstack != nullptr ? itemstack->copy() : nullptr;
		inventoryItemStacks[i] = itemstack1;
		for(size_t j = 0; j < crafters.size(); j++)
		{
			crafters[j]->sendSlotContents(this, i, itemstack1);
		}
	}
}

void Container::addCraftingToCrafters(ICrafting *icrafting)
{
	if (icrafting != nullptr)
	{
		crafters.push_back(icrafting);
	}
}

void Container::removeCraftingFromCrafters(ICrafting *icrafting)
{
	crafters.erase(std::remove(crafters.begin(), crafters.end(), icrafting), crafters.end());
}

bool Container::enchantItem(EntityPlayer *, int_t)
{
	return false;
}

Slot *Container::getSlot(int_t i)
{
	return i >= 0 && (std::size_t)i < slots.size() ? slots[(std::size_t)i] : nullptr;
}

bool Container::usesInventory(IInventory *iinventory)
{
	if (iinventory == nullptr)
		return false;
	for (Slot *slot : slots)
	{
		if (slot != nullptr && slot->getInventory() == iinventory)
			return true;
	}
	return false;
}

ItemStack *Container::getStackInSlot(int_t i)
{
	Slot *slot = slots[i];
	ItemStack *stack = slot != nullptr ? slot->getStack() : nullptr;
	return stack != nullptr ? stack->copy() : nullptr;
}

ItemStack *Container::slotClick(int_t slotIndex, int_t mouseButton, bool_t shiftClick, EntityPlayer *entityplayer)
{
	return slotClick(slotIndex, mouseButton, shiftClick ? 1 : 0, entityplayer);
}

ItemStack *Container::slotClick(int_t slotIndex, int_t mouseButton, int_t mode, EntityPlayer *entityplayer)
{
	ItemStack *itemstack = nullptr;
	InventoryPlayer *inventoryplayer = entityplayer != nullptr ? entityplayer->inventory : nullptr;
	if (inventoryplayer == nullptr)
		return nullptr;

	if (mode == 5)
	{
		const int_t oldState = field_94536_g;
		field_94536_g = func_94532_c(mouseButton);
		if ((oldState != 1 || field_94536_g != 2) && oldState != field_94536_g)
		{
			func_94533_d();
		}
		else if (inventoryplayer->getItemStack() == nullptr)
		{
			func_94533_d();
		}
		else if (field_94536_g == 0)
		{
			field_94535_f = func_94529_b(mouseButton);
			if (func_94528_d(field_94535_f))
			{
				field_94536_g = 1;
				field_94537_h.clear();
			}
			else
			{
				func_94533_d();
			}
		}
		else if (field_94536_g == 1)
		{
			Slot *slot = (slotIndex >= 0 && slotIndex < static_cast<int_t>(slots.size())) ? slots[slotIndex] : nullptr;
			if (slot != nullptr && func_94527_a(slot, inventoryplayer->getItemStack(), true) &&
			    slot->isItemValid(inventoryplayer->getItemStack()) &&
			    inventoryplayer->getItemStack()->stackSize > static_cast<int_t>(field_94537_h.size()) &&
			    canDragIntoSlot(slot))
			{
				field_94537_h.insert(slot);
			}
		}
		else if (field_94536_g == 2)
		{
			if (!field_94537_h.empty())
			{
				ItemStack *carriedCopy = inventoryplayer->getItemStack()->copy();
				int_t remaining = inventoryplayer->getItemStack()->stackSize;
				for (Slot *slot : field_94537_h)
				{
					if (slot != nullptr && func_94527_a(slot, inventoryplayer->getItemStack(), true) &&
					    slot->isItemValid(inventoryplayer->getItemStack()) &&
					    inventoryplayer->getItemStack()->stackSize >= static_cast<int_t>(field_94537_h.size()) &&
					    canDragIntoSlot(slot))
					{
						ItemStack *slotStack = carriedCopy->copy();
						const int_t baseCount = slot->getHasStack() ? slot->getStack()->stackSize : 0;
						func_94525_a(field_94537_h, field_94535_f, slotStack, baseCount);
						if (slotStack->stackSize > slotStack->getMaxStackSize())
							slotStack->stackSize = slotStack->getMaxStackSize();
						if (slotStack->stackSize > slot->getSlotStackLimit())
							slotStack->stackSize = slot->getSlotStackLimit();
						remaining -= slotStack->stackSize - baseCount;
						slot->putStack(slotStack);
					}
				}
				carriedCopy->stackSize = remaining;
				if (carriedCopy->stackSize <= 0)
				{
					delete carriedCopy;
					carriedCopy = nullptr;
				}
				ItemStack *oldCarried = inventoryplayer->getItemStack();
				inventoryplayer->setItemStack(carriedCopy);
				delete oldCarried;
			}
			func_94533_d();
		}
		else
		{
			func_94533_d();
		}
		return itemstack;
	}

	if (field_94536_g != 0)
	{
		func_94533_d();
		return itemstack;
	}

	if (mode == 6 && slotIndex >= 0)
	{
		Slot *slot = (slotIndex < static_cast<int_t>(slots.size())) ? slots[slotIndex] : nullptr;
		ItemStack *carried = inventoryplayer->getItemStack();
		if (carried != nullptr && (slot == nullptr || !slot->getHasStack() || !slot->canTakeStack(entityplayer)))
		{
			const int_t start = mouseButton == 0 ? 0 : static_cast<int_t>(slots.size()) - 1;
			const int_t step = mouseButton == 0 ? 1 : -1;
			for (int_t pass = 0; pass < 2; ++pass)
			{
				for (int_t idx = start; idx >= 0 && idx < static_cast<int_t>(slots.size()) && carried->stackSize < carried->getMaxStackSize(); idx += step)
				{
					Slot *targetSlot = slots[idx];
					if (targetSlot != nullptr && targetSlot->getHasStack() &&
					    func_94527_a(targetSlot, carried, true) && targetSlot->canTakeStack(entityplayer) &&
					    func_94530_a(carried, targetSlot) &&
					    (pass != 0 || targetSlot->getStack()->stackSize != targetSlot->getStack()->getMaxStackSize()))
					{
						const int_t takeCount = std::min(carried->getMaxStackSize() - carried->stackSize, targetSlot->getStack()->stackSize);
						ItemStack *taken = targetSlot->decrStackSize(takeCount);
						carried->stackSize += takeCount;
						if (taken == nullptr || taken->stackSize <= 0)
							targetSlot->putStack(nullptr);
						targetSlot->onPickupFromSlot(entityplayer, taken);
						delete taken;
					}
				}
			}
		}
		detectAndSendChanges();
		return itemstack;
	}

	const bool_t shiftClick = (mode == 1);
	if(mouseButton == 0 || mouseButton == 1)
	{
		if(slotIndex == -999)
		{
			if(inventoryplayer->getItemStack() != nullptr && slotIndex == -999)
			{
				if(mouseButton == 0)
				{
					entityplayer->dropPlayerItem(inventoryplayer->getItemStack());
					inventoryplayer->setItemStack(nullptr);
				}
				if(mouseButton == 1)
				{
					entityplayer->dropPlayerItem(inventoryplayer->getItemStack()->splitStack(1));
					if(inventoryplayer->getItemStack()->stackSize == 0)
					{
						// Java dropped the reference and let the GC take the
						// emptied stack. Here nothing else owns it.
						ItemStack *emptied = inventoryplayer->getItemStack();
						inventoryplayer->setItemStack(nullptr);
						delete emptied;
					}
				}
			}
		}
		else if(shiftClick)
		{
			Slot *from = slotIndex >= 0 && slotIndex < (int_t)slots.size() ? slots[slotIndex] : nullptr;
			ItemStack *transferred = from != nullptr && from->canTakeStack(entityplayer) ? transferStackInSlot(entityplayer, slotIndex) : nullptr;
			if(transferred != nullptr)
			{
				int_t itemId = transferred->itemID;
				itemstack = transferred;
				Slot *slot = slots[slotIndex];
				if(slot != nullptr && slot->getStack() != nullptr && slot->getStack()->itemID == itemId)
					retrySlotClick(slotIndex, mouseButton, shiftClick, entityplayer);
			}
		}
		else
		{
			Slot *slot = slots[slotIndex];
			if(slot != nullptr)
			{
				slot->onSlotChanged();
				ItemStack *itemstack2 = slot->getStack();
				ItemStack *itemstack3 = inventoryplayer->getItemStack();
				if(itemstack2 != nullptr)
				{
					itemstack = itemstack2->copy();
				}
				if(itemstack2 == nullptr)
				{
					if(itemstack3 != nullptr && slot->isItemValid(itemstack3))
					{
						int_t i1 = mouseButton != 0 ? 1 : itemstack3->stackSize;
						if(i1 > slot->getSlotStackLimit())
						{
							i1 = slot->getSlotStackLimit();
						}
						slot->putStack(itemstack3->splitStack(i1));
						if(itemstack3->stackSize == 0)
						{
							// The split went into the slot; what is left in hand is
							// an empty husk that only the GC used to clean up.
							inventoryplayer->setItemStack(nullptr);
							delete itemstack3;
							itemstack3 = nullptr;
						}
					}
				}
				else if(itemstack3 == nullptr)
				{
					// 1.5.2: SlotRepair refuses while the player lacks the XP.
					if(!slot->canTakeStack(entityplayer))
						return itemstack;
					int_t j1 = mouseButton != 0 ? (itemstack2->stackSize + 1) / 2 : itemstack2->stackSize;
					ItemStack *itemstack5 = slot->decrStackSize(j1);
					inventoryplayer->setItemStack(itemstack5);
					if(itemstack2->stackSize == 0)
					{
						slot->putStack(nullptr);
					}
					slot->onPickupFromSlot(entityplayer, inventoryplayer->getItemStack());
				}
				else if(slot->isItemValid(itemstack3))
				{
					if(itemstack2->itemID != itemstack3->itemID || itemstack2->getHasSubtypes() && itemstack2->getItemDamage() != itemstack3->getItemDamage() || !ItemStack::func_46154_a(itemstack2, itemstack3))
					{
						if(itemstack3->stackSize <= slot->getSlotStackLimit())
						{
							ItemStack *itemstack4 = slot->takeStack();
							slot->putStack(itemstack3);
							inventoryplayer->setItemStack(itemstack4);
						}
					}
					else
					{
						int_t k1 = mouseButton != 0 ? 1 : itemstack3->stackSize;
						if(k1 > slot->getSlotStackLimit() - itemstack2->stackSize)
						{
							k1 = slot->getSlotStackLimit() - itemstack2->stackSize;
						}
						if(k1 > itemstack3->getMaxStackSize() - itemstack2->stackSize)
						{
							k1 = itemstack3->getMaxStackSize() - itemstack2->stackSize;
						}
						// splitStack() allocates; Java discarded the result because the
						// count is merged into itemstack2 below, so the object is dead
						// on arrival and has to be freed here.
						delete itemstack3->splitStack(k1);
						if(itemstack3->stackSize == 0)
						{
							inventoryplayer->setItemStack(nullptr);
							delete itemstack3;
							itemstack3 = nullptr;
						}
						itemstack2->stackSize += k1;
					}
				}
				else if(itemstack2->itemID == itemstack3->itemID && itemstack3->getMaxStackSize() > 1 && (!itemstack2->getHasSubtypes() || itemstack2->getItemDamage() == itemstack3->getItemDamage()) && ItemStack::func_46154_a(itemstack2, itemstack3))
				{
					int_t l1 = itemstack2->stackSize;
					if(l1 > 0 && l1 + itemstack3->stackSize <= itemstack3->getMaxStackSize())
					{
						itemstack3->stackSize += l1;
						ItemStack *removed = slot->decrStackSize(l1);
						slot->onPickupFromSlot(entityplayer, inventoryplayer->getItemStack());
						delete removed;
					}
				}
			}
		}
	}
	return itemstack;
}

void Container::retrySlotClick(int_t slotIndex, int_t mouseButton, bool shiftClick, EntityPlayer *entityplayer)
{
	ItemStack *result = slotClick(slotIndex, mouseButton, shiftClick, entityplayer);
	delete result;
}

void Container::onCraftGuiClosed(EntityPlayer *entityplayer)
{
	InventoryPlayer *inventoryplayer = entityplayer->inventory;
	if(inventoryplayer->getItemStack() != nullptr)
	{
		entityplayer->dropPlayerItem(inventoryplayer->getItemStack());
		inventoryplayer->setItemStack(nullptr);
	}
}

void Container::func_94533_d()
{
	field_94536_g = 0;
	field_94537_h.clear();
}

bool Container::func_94527_a(Slot *slot, ItemStack *stack, bool stackSizeMatters)
{
	bool canAdd = (slot == nullptr || !slot->getHasStack());
	if (slot != nullptr && slot->getHasStack() && stack != nullptr &&
	    stack->isItemEqual(slot->getStack()) && ItemStack::func_46154_a(slot->getStack(), stack))
	{
		const int_t extra = stackSizeMatters ? 0 : stack->stackSize;
		canAdd |= (slot->getStack()->stackSize + extra <= stack->getMaxStackSize());
	}
	return canAdd;
}

void Container::func_94525_a(const std::set<Slot *> &dragSlots, int_t limit, ItemStack *stack, int_t baseSize)
{
	if (stack == nullptr)
		return;
	switch (limit)
	{
	case 0:
		stack->stackSize = MathHelper::floor_float(static_cast<float>(stack->stackSize) / static_cast<float>(dragSlots.size()));
		break;
	case 1:
		stack->stackSize = 1;
		break;
	}
	stack->stackSize += baseSize;
}

bool Container::func_94530_a(ItemStack *, Slot *)
{
	return true;
}

bool Container::canDragIntoSlot(Slot *)
{
	return true;
}

void Container::onCraftMatrixChanged(IInventory *iinventory)
{
	updateCraftingResults();
}

void Container::putStackInSlot(int_t i, ItemStack *itemstack)
{
	Slot *slot = getSlot(i);
	if (slot == nullptr)
	{
		delete itemstack;
		return;
	}
	slot->putStack(itemstack);
}

void Container::putStacksInSlots(ItemStack **aitemstack, int_t length)
{
	if (aitemstack == nullptr || length <= 0)
		return;
	for(int_t i = 0; i < length; i++)
	{
		Slot *slot = getSlot(i);
		if (slot != nullptr)
			slot->putStack(aitemstack[i]);
		else
			delete aitemstack[i];
	}
}

void Container::updateProgressBar(int_t id, int_t value)
{
}

short_t Container::getNextTransactionID(InventoryPlayer *inventoryplayer)
{
	transactionID++;
	return transactionID;
}

void Container::updateTransaction(short_t transactionID)
{
}

void Container::onTransactionResponse(short_t transactionID)
{
}

bool Container::mergeItemStack(ItemStack *itemstack, int_t startSlot, int_t endSlot, bool_t lastToFirst)
{
	bool changed = false;
	int_t slotIndex = lastToFirst ? endSlot - 1 : startSlot;

	if(itemstack->isStackable())
	{
		while(itemstack->stackSize > 0 && (!lastToFirst && slotIndex < endSlot || lastToFirst && slotIndex >= startSlot))
		{
			Slot *slot = slots[slotIndex];
			ItemStack *slotStack = slot->getStack();
			if(slotStack != nullptr && slotStack->itemID == itemstack->itemID &&
			   (!itemstack->getHasSubtypes() || itemstack->getItemDamage() == slotStack->getItemDamage()) &&
			   ItemStack::func_46154_a(itemstack, slotStack))
			{
				int_t combinedSize = slotStack->stackSize + itemstack->stackSize;
				if(combinedSize <= itemstack->getMaxStackSize())
				{
					itemstack->stackSize = 0;
					slotStack->stackSize = combinedSize;
					slot->onSlotChanged();
					changed = true;
				}
				else if(slotStack->stackSize < itemstack->getMaxStackSize())
				{
					itemstack->stackSize -= itemstack->getMaxStackSize() - slotStack->stackSize;
					slotStack->stackSize = itemstack->getMaxStackSize();
					slot->onSlotChanged();
					changed = true;
				}
			}

			slotIndex += lastToFirst ? -1 : 1;
		}
	}

	if(itemstack->stackSize > 0)
	{
		slotIndex = lastToFirst ? endSlot - 1 : startSlot;
		while(!lastToFirst && slotIndex < endSlot || lastToFirst && slotIndex >= startSlot)
		{
			Slot *slot = slots[slotIndex];
			if(slot->getStack() == nullptr)
			{
				slot->putStack(itemstack->copy());
				slot->onSlotChanged();
				itemstack->stackSize = 0;
				changed = true;
				break;
			}

			slotIndex += lastToFirst ? -1 : 1;
		}
	}

	return changed;
}


int_t Container::calcRedstoneFromInventory(IInventory *inventory)
{
	if (inventory == nullptr)
	{
		return 0;
	}

	int_t itemsCount = 0;
	float_t fillRatio = 0.0f;
	const int_t size = inventory->getSizeInventory();
	if (size <= 0)
	{
		return 0;
	}

	for (int_t slot = 0; slot < size; ++slot)
	{
		ItemStack *stack = inventory->getStackInSlot(slot);
		if (stack != nullptr)
		{
			const int_t limit = std::min(inventory->getInventoryStackLimit(), stack->getMaxStackSize());
			if (limit > 0)
			{
				fillRatio += (float_t)stack->stackSize / (float_t)limit;
			}
			itemsCount++;
		}
	}

	fillRatio /= (float_t)size;
	return MathHelper::floor_float(fillRatio * 14.0f) + (itemsCount > 0 ? 1 : 0);
}
