#pragma once
#include "GuiContainer.h"
class EntityPlayer;
class GuiButton;
class IMerchant;
class InventoryPlayer;
class MerchantRecipeList;
class GuiMerchant : public GuiContainer
{
public:
	GuiMerchant(InventoryPlayer *inventory, IMerchant *merchant, MerchantRecipeList *recipes, EntityPlayer *player = nullptr);
	void initGui() override;
protected:
	void drawGuiContainerForegroundLayer() override;
	void drawGuiContainerBackgroundLayer(float_t partialTick) override;
	void actionPerformed(GuiButton *button) override;
private:
	IMerchant *merchant;
	MerchantRecipeList *recipes;
	int_t recipeIndex;
};
