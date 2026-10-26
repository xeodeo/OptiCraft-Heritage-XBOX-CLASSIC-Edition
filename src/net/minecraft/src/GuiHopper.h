#pragma once

#include "GuiContainer.h"

class InventoryPlayer;
class IInventory;

// net.minecraft.src.GuiHopper
class GuiHopper : public GuiContainer
{
private:
	IInventory *playerInventory;
	IInventory *hopperInventory;

public:
	GuiHopper(InventoryPlayer *player, IInventory *hopperInv, EntityPlayer *entityPlayer = nullptr);

protected:
	void drawGuiContainerForegroundLayer() override;
	void drawGuiContainerBackgroundLayer(float_t partialTick) override;
};
