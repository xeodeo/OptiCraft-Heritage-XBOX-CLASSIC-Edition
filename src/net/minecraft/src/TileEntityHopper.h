#pragma once

#include "TileEntity.h"
#include "Hopper.h"
#include "IInventory.h"
#include <string>

class ItemStack;
class EntityItem;

// net.minecraft.src.TileEntityHopper
class TileEntityHopper : public TileEntity, public Hopper, public IInventory
{
private:
	ItemStack *hopperItemStacks[5];
	std::string inventoryName;
	int_t transferCooldown = -1;

public:
	TileEntityHopper();
	~TileEntityHopper() override;

	void readFromNBT(NBTTagCompound *nbttagcompound) override;
	void writeToNBT(NBTTagCompound *nbttagcompound) override;

	int_t getSizeInventory() override;
	ItemStack *getStackInSlot(int_t i) override;
	ItemStack *decrStackSize(int_t i, int_t j) override;
	ItemStack *getStackInSlotOnClosing(int_t i) override;
	void setInventorySlotContents(int_t i, ItemStack *itemstack) override;
	std::string getInvName() override;
	int_t getInventoryStackLimit() override;
	void onInventoryChanged() override;
	bool canInteractWith(EntityPlayer *entityplayer) override;
	void openChest() override {}
	void closeChest() override {}
	void updateEntity() override;

	World *getWorldObj() override;
	double getXPos() override;
	double getYPos() override;
	double getZPos() override;
	IInventory *getHopperInventory() override { return this; }

	void setTransferCooldown(int_t cooldown);
	bool isCoolingDown() const;
	bool func_98045_j();

	bool isInvNameLocalized() const;
	void setInventoryName(const std::string &name);

	static bool suckItemsIntoHopper(Hopper *hopper);
	static bool func_96114_a(IInventory *inv, EntityItem *entityItem);
	static ItemStack *insertStack(IInventory *inv, ItemStack *stack, int_t side);
	static IInventory *getInventoryAtLocation(World *world, double x, double y, double z);
	static IInventory *getInventoryAboveHopper(Hopper *hopper);
	static EntityItem *func_96119_a(World *world, double x, double y, double z);

private:
	bool insertItemToInventory();
	IInventory *getOutputInventory();
	static bool func_102012_a(Hopper *hopper, IInventory *inv, int_t slot, int_t side);
	static bool func_102015_a(IInventory *inv, ItemStack *stack, int_t slot, int_t side);
	static bool canExtractItemFromInventory(IInventory *inv, ItemStack *stack, int_t slot, int_t side);
	static ItemStack *func_102014_c(IInventory *inv, ItemStack *stack, int_t slot, int_t side);
	static bool areItemStacksEqualItem(ItemStack *a, ItemStack *b);
};
