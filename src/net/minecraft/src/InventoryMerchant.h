#pragma once

#include "IInventory.h"

class EntityPlayer;
class IMerchant;
class ItemStack;
class MerchantRecipe;

class InventoryMerchant : public IInventory
{
public:
	InventoryMerchant(EntityPlayer *player, IMerchant *merchant);
	~InventoryMerchant() override;
	int_t getSizeInventory() override;
	ItemStack *getStackInSlot(int_t slot) override;
	ItemStack *getStackInSlotOnClosing(int_t slot) override;
	ItemStack *decrStackSize(int_t slot, int_t count) override;
	void setInventorySlotContents(int_t slot, ItemStack *stack) override;
	std::string getInvName() override;
	int_t getInventoryStackLimit() override;
	void onInventoryChanged() override;
	bool canInteractWith(EntityPlayer *player) override;
	bool isStackValidForSlot(int_t slot, ItemStack *stack) override;
	void resetRecipeAndSlots();
	MerchantRecipe *getCurrentRecipe() const;
	void setCurrentRecipeIndex(int_t index);

private:
	EntityPlayer *thePlayer;
	IMerchant *theMerchant;
	ItemStack *inventory[3]{};
	MerchantRecipe *currentRecipe = nullptr;
	int_t currentRecipeIndex = 0;
};
