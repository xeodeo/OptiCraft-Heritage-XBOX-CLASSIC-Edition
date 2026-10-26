#pragma once

#include "Container.h"

class EntityPlayer;
class IMerchant;
class InventoryMerchant;
class InventoryPlayer;
class World;

class ContainerMerchant : public Container
{
public:
	ContainerMerchant(InventoryPlayer *inventory, IMerchant *merchant, World *world);
	~ContainerMerchant() override;
	InventoryMerchant *getMerchantInventory() const;
	void setCurrentRecipeIndex(int_t index);
	void onCraftMatrixChanged(IInventory *inventory) override;
	void onCraftGuiClosed(EntityPlayer *player) override;
	bool isUsableByPlayer(EntityPlayer *player) override;
	ItemStack *getStackInSlot(int_t slotIndex) override;

private:
	IMerchant *theMerchant;
	InventoryMerchant *merchantInventory;
	World *theWorld;
};
