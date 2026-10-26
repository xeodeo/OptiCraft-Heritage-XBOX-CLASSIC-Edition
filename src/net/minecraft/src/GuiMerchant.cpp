#include "GuiMerchant.h"
#include <cstdio>
#include "ContainerMerchant.h"
#include "EntityPlayer.h"
#include "FontRenderer.h"
#include "GuiButton.h"
#include "GuiButtonMerchant.h"
#include "IMerchant.h"
#include "InventoryPlayer.h"
#include "MerchantRecipeList.h"
#include "Minecraft.h"
#include "RenderEngine.h"
#include "StatCollector.h"
#include "platform/RenderAPI.h"

GuiMerchant::GuiMerchant(InventoryPlayer *inv, IMerchant *m, MerchantRecipeList *list, EntityPlayer *player)
	: GuiContainer(new ContainerMerchant(inv, m, player ? player->worldObj : nullptr), true, player ? player : (inv ? inv->player : nullptr)), merchant(m), recipes(list), recipeIndex(0)
{
	xSize = 176; ySize = 166;
}
void GuiMerchant::initGui()
{
	GuiContainer::initGui();
	controlList.push_back(new GuiButtonMerchant(0, width / 2 - 8, height / 2 - 54, -1, "<"));
	controlList.push_back(new GuiButtonMerchant(1, width / 2 + 34, height / 2 - 54, 1, ">"));
}
void GuiMerchant::drawGuiContainerForegroundLayer()
{
	fontRenderer->drawString(StatCollector::translateToLocal("mob.villager"), 56, 6, 0x404040);
	fontRenderer->drawString(StatCollector::translateToLocal("container.inventory"), 8, ySize - 94, 0x404040);
	if (recipes && recipes->size() > 0)
	{
		char count[24]; std::snprintf(count, sizeof(count), "%d/%d", recipeIndex + 1, recipes->size());
		fontRenderer->drawString(count, 82, 39, 0x404040);
	}
}
void GuiMerchant::drawGuiContainerBackgroundLayer(float_t)
{
	int_t texture = mc->renderEngine->getTexture("/gui/trading.png");
	renderColor4f(1, 1, 1, 1); mc->renderEngine->bindTexture(texture);
	int_t x = (width - xSize) / 2, y = (height - ySize) / 2;
	drawTexturedModalRect(x, y, 0, 0, xSize, ySize);
}
void GuiMerchant::actionPerformed(GuiButton *button)
{
	auto *merchantButton = dynamic_cast<GuiButtonMerchant *>(button);
	auto *container = dynamic_cast<ContainerMerchant *>(inventorySlots);
	if (!merchantButton || !container || !recipes || recipes->size() == 0) return;
	recipeIndex = (recipeIndex + merchantButton->recipeDelta + recipes->size()) % recipes->size();
	container->setCurrentRecipeIndex(recipeIndex);
}
