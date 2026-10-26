#include "ContainerHopper.h"
#include "Slot.h"
#include "IInventory.h"
#include "InventoryPlayer.h"
#include "ItemStack.h"
#include "EntityPlayer.h"

ContainerHopper::ContainerHopper(InventoryPlayer *playerInventory, IInventory *hopperInv)
	: hopperInventory(hopperInv)
{
	if (hopperInventory != nullptr)
	{
		hopperInventory->openChest();
		for (int_t slot = 0; slot < hopperInventory->getSizeInventory(); ++slot)
		{
			addSlot(new Slot(hopperInventory, slot, 44 + slot * 18, 20));
		}
	}

	const int_t playerInvY = 51;
	for (int_t row = 0; row < 3; ++row)
	{
		for (int_t col = 0; col < 9; ++col)
		{
			addSlot(new Slot(playerInventory, col + row * 9 + 9, 8 + col * 18, row * 18 + playerInvY));
		}
	}

	for (int_t col = 0; col < 9; ++col)
	{
		addSlot(new Slot(playerInventory, col, 8 + col * 18, 58 + playerInvY));
	}
}

ContainerHopper::~ContainerHopper()
{
}

bool ContainerHopper::isUsableByPlayer(EntityPlayer *entityplayer)
{
	return hopperInventory != nullptr && hopperInventory->canInteractWith(entityplayer);
}

ItemStack *ContainerHopper::getStackInSlot(int_t i)
{
	ItemStack *result = nullptr;
	if (i >= 0 && i < static_cast<int_t>(slots.size()))
	{
		Slot *slot = slots[i];
		if (slot != nullptr && slot->getHasStack())
		{
			ItemStack *slotStack = slot->getStack();
			result = slotStack->copy();
			const int_t hopperSize = hopperInventory != nullptr ? hopperInventory->getSizeInventory() : 5;
			if (i < hopperSize)
			{
				if (!mergeItemStack(slotStack, hopperSize, slots.size(), true))
				{
					delete result;
					return nullptr;
				}
			}
			else if (!mergeItemStack(slotStack, 0, hopperSize, false))
			{
				delete result;
				return nullptr;
			}

			if (slotStack->stackSize == 0)
			{
				slot->putStack(nullptr);
			}
			else
			{
				slot->onSlotChanged();
			}
		}
	}
	return result;
}

void ContainerHopper::onCraftGuiClosed(EntityPlayer *entityplayer)
{
	Container::onCraftGuiClosed(entityplayer);
	if (hopperInventory != nullptr)
	{
		hopperInventory->closeChest();
	}
}
