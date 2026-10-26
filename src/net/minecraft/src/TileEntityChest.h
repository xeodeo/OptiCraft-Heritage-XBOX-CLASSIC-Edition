#pragma once

#include <string>

#include "IInventory.h"
#include "TileEntity.h"
#include "java/Type.h"

class EntityPlayer;
class ItemStack;
class NBTTagCompound;

// net.minecraft.src.TileEntityChest
class TileEntityChest : public TileEntity, public IInventory
{
public:
	explicit TileEntityChest(int_t type = -1);
	~TileEntityChest() override;

	int_t getSizeInventory() override;
	ItemStack *getStackInSlot(int_t i) override;
	ItemStack *getStackInSlotOnClosing(int_t i) override;
	ItemStack *decrStackSize(int_t i, int_t j) override;
	void setInventorySlotContents(int_t i, ItemStack *itemstack) override;
	std::string getInvName() override;
	void readFromNBT(NBTTagCompound *nbttagcompound) override;
	void writeToNBT(NBTTagCompound *nbttagcompound) override;
	int_t getInventoryStackLimit() override;
	bool canInteractWith(EntityPlayer *entityplayer) override;
	void onInventoryChanged() override { TileEntity::onInventoryChanged(); }
	void openChest() override;
	void closeChest() override;

	void updateContainingBlockInfo() override;
	void updateEntity() override;
	void onTileEntityPowered(int_t eventId, int_t eventData) override;
	void invalidate() override;

	void checkForAdjacentChests();
	int_t func_98041_l();
	bool func_94044_a(int_t x, int_t y, int_t z);

	bool adjacentChestChecked;
	TileEntityChest *adjacentChestZNeg;
	TileEntityChest *adjacentChestXPos;
	TileEntityChest *adjacentChestXNeg;
	TileEntityChest *adjacentChestZPos;
	float lidAngle;
	float prevLidAngle;
	int_t numUsingPlayers;

private:
	int_t ticksSinceSync;
	int_t field_94046_i;
	ItemStack *chestContents[36];
};
