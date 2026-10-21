#pragma once

#include "TileEntity.h"
#include "IInventory.h"

// net.minecraft.src.TileEntityBeacon
class TileEntityBeacon : public TileEntity, public IInventory
{
public:
	TileEntityBeacon();
	~TileEntityBeacon() override;

	void updateEntity() override;
	float func_82125_v_();

	int_t getLevels() const { return levels; }
	void setLevels(int_t l) { levels = l; }
	int_t getPrimaryEffect() const { return primaryEffect; }
	void setPrimaryEffect(int_t eff) { primaryEffect = eff; }
	int_t getSecondaryEffect() const { return secondaryEffect; }
	void setSecondaryEffect(int_t eff) { secondaryEffect = eff; }

	void readFromNBT(NBTTagCompound *nbttagcompound) override;
	void writeToNBT(NBTTagCompound *nbttagcompound) override;

	int_t getSizeInventory() override { return 1; }
	ItemStack *getStackInSlot(int_t i) override;
	ItemStack *decrStackSize(int_t i, int_t j) override;
	ItemStack *getStackInSlotOnClosing(int_t i) override;
	void setInventorySlotContents(int_t i, ItemStack *itemstack) override;
	std::string getInvName() override { return "container.beacon"; }
	int_t getInventoryStackLimit() override { return 1; }
	void onInventoryChanged() override {}
	bool canInteractWith(EntityPlayer *entityplayer) override;

private:
	void updateState();
	void addEffectsToPlayers();

	long_t field_82137_b = 0;
	float field_82138_c = 0.0f;
	bool isBeaconActive = false;
	int_t levels = -1;
	int_t primaryEffect = 0;
	int_t secondaryEffect = 0;
	ItemStack *payment = nullptr;
};
