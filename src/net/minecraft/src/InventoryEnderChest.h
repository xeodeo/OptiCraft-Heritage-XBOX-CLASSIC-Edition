#pragma once

#include "InventoryBasic.h"

class TileEntityEnderChest;
class NBTTagList;

// net.minecraft.src.InventoryEnderChest
class InventoryEnderChest : public InventoryBasic
{
public:
	InventoryEnderChest();

	void setAssociatedChest(TileEntityEnderChest *chest);
	void loadInventoryFromNBT(NBTTagList *list);
	NBTTagList *saveInventoryToNBT();

	bool canInteractWith(EntityPlayer *player) override;
	void openChest() override;
	void closeChest() override;

private:
	TileEntityEnderChest *associatedChest = nullptr;
};
