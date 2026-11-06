#pragma once

#include <string>
#include "GuiContainer.h"

class EntityPlayer;
class GuiButton;
class GuiButtonMerchant;
class IMerchant;
class InventoryPlayer;
class ItemStack;
class World;

// Called by a villager that is deleted while a trade screen still points at it.
void merchantScreensForget(IMerchant *merchant);

// net.minecraft.src.GuiMerchant
class GuiMerchant : public GuiContainer
{
public:
	GuiMerchant(InventoryPlayer *inventory, IMerchant *merchant, World *world, const std::string &title = "", EntityPlayer *player = nullptr);
	~GuiMerchant() override = default;

	void initGui() override;
	void updateScreen() override;
	void drawScreen(int_t mouseX, int_t mouseY, float_t partialTick) override;
	IMerchant *getIMerchant() const;
	void detachMerchant();

protected:
	void drawGuiContainerForegroundLayer() override;
	void drawGuiContainerBackgroundLayer(float_t partialTick) override;
	void actionPerformed(GuiButton *button) override;

private:
	bool isPointInRegion(int_t x, int_t y, int_t w, int_t h, int_t pointX, int_t pointY);
	void drawItemStackTooltip(ItemStack *stack, int_t mouseX, int_t mouseY);

	IMerchant *theIMerchant;
	GuiButtonMerchant *nextRecipeButtonIndex;
	GuiButtonMerchant *previousRecipeButtonIndex;
	int_t currentRecipeIndex;
	std::string titleString;
};
