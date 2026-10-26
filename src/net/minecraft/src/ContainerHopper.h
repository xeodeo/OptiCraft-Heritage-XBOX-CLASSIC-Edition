#pragma once

#include "Container.h"

class IInventory;
class InventoryPlayer;

// net.minecraft.src.ContainerHopper
class ContainerHopper : public Container
{
private:
	IInventory *hopperInventory;

public:
	ContainerHopper(InventoryPlayer *playerInventory, IInventory *hopperInv);
	~ContainerHopper() override;

	bool isUsableByPlayer(EntityPlayer *entityplayer) override;
	ItemStack *getStackInSlot(int_t i) override;
	void onCraftGuiClosed(EntityPlayer *entityplayer) override;
};
