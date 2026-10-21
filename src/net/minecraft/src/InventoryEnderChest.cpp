#include "InventoryEnderChest.h"

#include "ItemStack.h"
#include "NBTTagCompound.h"
#include "NBTTagList.h"
#include "TileEntityEnderChest.h"

InventoryEnderChest::InventoryEnderChest()
	: InventoryBasic("container.enderchest", 27, true)
{
}

void InventoryEnderChest::setAssociatedChest(TileEntityEnderChest *chest)
{
	associatedChest = chest;
}

void InventoryEnderChest::loadInventoryFromNBT(NBTTagList *list)
{
	if (list == nullptr)
	{
		return;
	}
	for (int_t i = 0; i < getSizeInventory(); ++i)
	{
		setInventorySlotContents(i, nullptr);
	}
	for (int_t i = 0; i < list->tagCount(); ++i)
	{
		NBTTagCompound *tag = dynamic_cast<NBTTagCompound *>(list->tagAt(i));
		if (tag != nullptr)
		{
			const int_t slot = tag->getByte("Slot") & 255;
			if (slot >= 0 && slot < getSizeInventory())
			{
				setInventorySlotContents(slot, ItemStack::loadItemStackFromNBT(tag));
			}
		}
	}
}

NBTTagList *InventoryEnderChest::saveInventoryToNBT()
{
	NBTTagList *list = new NBTTagList();
	for (int_t i = 0; i < getSizeInventory(); ++i)
	{
		ItemStack *stack = getStackInSlot(i);
		if (stack != nullptr)
		{
			NBTTagCompound *tag = new NBTTagCompound();
			tag->setByte("Slot", static_cast<byte_t>(i));
			stack->writeToNBT(tag);
			list->appendTag(tag);
		}
	}
	return list;
}

bool InventoryEnderChest::canInteractWith(EntityPlayer *player)
{
	if (associatedChest != nullptr && !associatedChest->isUseableByPlayer(player))
	{
		return false;
	}
	return InventoryBasic::canInteractWith(player);
}

void InventoryEnderChest::openChest()
{
	if (associatedChest != nullptr)
	{
		associatedChest->openChest();
	}
	InventoryBasic::openChest();
}

void InventoryEnderChest::closeChest()
{
	if (associatedChest != nullptr)
	{
		associatedChest->closeChest();
	}
	InventoryBasic::closeChest();
	associatedChest = nullptr;
}
