#include "GuiMerchant.h"

#include <algorithm>
#include <vector>

#include "ContainerMerchant.h"
#include "EntityPlayer.h"
#include "EntityPlayerSP.h"
#include "FontRenderer.h"
#include "GuiButton.h"
#include "GuiButtonMerchant.h"
#include "IMerchant.h"
#include "InventoryPlayer.h"
#include "ItemStack.h"
#include "EnumRarity.h"
#include "MerchantRecipe.h"
#include "MerchantRecipeList.h"
#include "Minecraft.h"
#include "RenderEngine.h"
#include "RenderHelper.h"
#include "RenderItem.h"
#include "StatCollector.h"
#include "platform/Input.h"
#include "platform/RenderAPI.h"
#include "legacy/LegacyTradingScreen.h"

GuiMerchant::GuiMerchant(InventoryPlayer *inventory, IMerchant *merchant, World *world, const std::string &title, EntityPlayer *player)
	: GuiContainer(new ContainerMerchant(inventory, merchant, world), true, player ? player : (inventory ? inventory->player : nullptr)),
	  theIMerchant(merchant), nextRecipeButtonIndex(nullptr), previousRecipeButtonIndex(nullptr), currentRecipeIndex(0), titleString(title)
{
	xSize = 176;
	ySize = 166;
	if (titleString.empty())
		titleString = StatCollector::translateToLocal("entity.Villager.name");
	if (titleString.empty() || titleString == "entity.Villager.name")
		titleString = StatCollector::translateToLocal("mob.villager");
}

void GuiMerchant::initGui()
{
	GuiContainer::initGui();
	int_t x = (width - xSize) / 2;
	int_t y = (height - ySize) / 2;
	nextRecipeButtonIndex = new GuiButtonMerchant(1, x + 120 + 27, y + 24 - 1, true);
	previousRecipeButtonIndex = new GuiButtonMerchant(2, x + 36 - 19, y + 24 - 1, false);
	controlList.push_back(nextRecipeButtonIndex);
	controlList.push_back(previousRecipeButtonIndex);
	nextRecipeButtonIndex->enabled = false;
	previousRecipeButtonIndex->enabled = false;
}

void GuiMerchant::drawGuiContainerForegroundLayer()
{
	int_t titleWidth = fontRenderer->getStringWidth(titleString);
	fontRenderer->drawString(titleString, xSize / 2 - titleWidth / 2, 6, 0x404040);
	fontRenderer->drawString(StatCollector::translateToLocal("container.inventory"), 8, ySize - 96 + 2, 0x404040);
}

void GuiMerchant::updateScreen()
{
	GuiContainer::updateScreen();
	EntityPlayer *player = getContainerPlayer();
	if (player == nullptr)
		player = mc ? mc->thePlayer : nullptr;
	// Java's server closes the container once InventoryMerchant.isUseableByPlayer fails
	// (EntityAITradePlayer clears the customer when the player walks away or the villager is hit).
	if (mc != nullptr && (theIMerchant == nullptr || theIMerchant->getCustomer() != player))
	{
		if (mc->isSplitScreenActive())
			mc->closePlayerScreen(getOwnerPlayerIndex());
		else
			mc->displayGuiScreen(nullptr);
		return;
	}
	MerchantRecipeList *recipeList = theIMerchant ? theIMerchant->getRecipes(player) : nullptr;
	if (recipeList != nullptr)
	{
		if (nextRecipeButtonIndex != nullptr)
			nextRecipeButtonIndex->enabled = currentRecipeIndex < recipeList->size() - 1;
		if (previousRecipeButtonIndex != nullptr)
			previousRecipeButtonIndex->enabled = currentRecipeIndex > 0;
	}

#if defined(PS2_PLATFORM) || defined(WII_PLATFORM) || defined(XBOX_PLATFORM)
	const int pIdx = getOwnerPlayerIndex();
	const PlatformTextInputSnapshot input = platformTextInputSnapshot(pIdx);
	if (input.connected)
	{
		if ((input.pressed & PLATFORM_TEXT_PREV_PAGE) && previousRecipeButtonIndex && previousRecipeButtonIndex->enabled)
		{
			actionPerformed(previousRecipeButtonIndex);
		}
		else if ((input.pressed & PLATFORM_TEXT_NEXT_PAGE) && nextRecipeButtonIndex && nextRecipeButtonIndex->enabled)
		{
			actionPerformed(nextRecipeButtonIndex);
		}
	}
#endif
}

void GuiMerchant::actionPerformed(GuiButton *button)
{
	bool changed = false;
	if (button == nextRecipeButtonIndex)
	{
		++currentRecipeIndex;
		changed = true;
	}
	else if (button == previousRecipeButtonIndex)
	{
		--currentRecipeIndex;
		changed = true;
	}

	if (changed)
	{
		auto *container = dynamic_cast<ContainerMerchant *>(inventorySlots);
		if (container)
			container->setCurrentRecipeIndex(currentRecipeIndex);
	}
}

void GuiMerchant::drawGuiContainerBackgroundLayer(float_t)
{
	int_t texture = mc->renderEngine->getTexture("/gui/trading.png");
	renderColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	mc->renderEngine->bindTexture(texture);
	int_t x = (width - xSize) / 2;
	int_t y = (height - ySize) / 2;
	drawTexturedModalRect(x, y, 0, 0, xSize, ySize);

	EntityPlayer *player = getContainerPlayer();
	if (player == nullptr)
		player = mc ? mc->thePlayer : nullptr;
	MerchantRecipeList *recipeList = theIMerchant ? theIMerchant->getRecipes(player) : nullptr;
	if (recipeList != nullptr && !recipeList->empty())
	{
		int_t index = currentRecipeIndex;
		if (index >= 0 && index < recipeList->size())
		{
			MerchantRecipe *recipe = recipeList->get(index);
			if (recipe != nullptr && recipe->isRecipeDisabled())
			{
				mc->renderEngine->bindTexture(texture);
				renderColor4f(1.0f, 1.0f, 1.0f, 1.0f);
				renderDisable(RenderCapability::Lighting);
				drawTexturedModalRect(guiLeft + 83, guiTop + 21, 212, 0, 28, 21);
				drawTexturedModalRect(guiLeft + 83, guiTop + 51, 212, 0, 28, 21);
			}
		}
	}
}

void GuiMerchant::drawScreen(int_t mouseX, int_t mouseY, float_t partialTick)
{
	GuiContainer::drawScreen(mouseX, mouseY, partialTick);

	EntityPlayer *player = getContainerPlayer();
	if (player == nullptr)
		player = mc ? mc->thePlayer : nullptr;
	MerchantRecipeList *recipeList = theIMerchant ? theIMerchant->getRecipes(player) : nullptr;
	if (recipeList != nullptr && !recipeList->empty())
	{
		int_t x = (width - xSize) / 2;
		int_t y = (height - ySize) / 2;
		int_t index = currentRecipeIndex;
		if (index >= 0 && index < recipeList->size())
		{
			MerchantRecipe *recipe = recipeList->get(index);
			if (recipe != nullptr)
			{
				renderPushMatrix();
				ItemStack *firstBuy = recipe->getItemToBuy();
				ItemStack *secondBuy = recipe->getSecondItemToBuy();
				ItemStack *sell = recipe->getItemToSell();

				RenderHelper::enableGUIStandardItemLighting();
				renderDisable(RenderCapability::Lighting);
				renderEnable(RenderCapability::RescaleNormal);
				renderEnable(RenderCapability::ColorMaterial);
				renderEnable(RenderCapability::Lighting);

				itemRenderer->zLevel = 100.0f;
				if (firstBuy != nullptr)
				{
					itemRenderer->renderItemIntoGUI(fontRenderer, mc->renderEngine, firstBuy, x + 36, y + 24);
					itemRenderer->renderItemOverlayIntoGUI(fontRenderer, mc->renderEngine, firstBuy, x + 36, y + 24);
				}
				if (secondBuy != nullptr)
				{
					itemRenderer->renderItemIntoGUI(fontRenderer, mc->renderEngine, secondBuy, x + 62, y + 24);
					itemRenderer->renderItemOverlayIntoGUI(fontRenderer, mc->renderEngine, secondBuy, x + 62, y + 24);
				}
				if (sell != nullptr)
				{
					itemRenderer->renderItemIntoGUI(fontRenderer, mc->renderEngine, sell, x + 120, y + 24);
					itemRenderer->renderItemOverlayIntoGUI(fontRenderer, mc->renderEngine, sell, x + 120, y + 24);
				}
				itemRenderer->zLevel = 0.0f;

				renderDisable(RenderCapability::Lighting);

				if (firstBuy != nullptr && isPointInRegion(36, 24, 16, 16, mouseX, mouseY))
				{
					drawItemStackTooltip(firstBuy, mouseX, mouseY);
				}
				else if (secondBuy != nullptr && isPointInRegion(62, 24, 16, 16, mouseX, mouseY))
				{
					drawItemStackTooltip(secondBuy, mouseX, mouseY);
				}
				else if (sell != nullptr && isPointInRegion(120, 24, 16, 16, mouseX, mouseY))
				{
					drawItemStackTooltip(sell, mouseX, mouseY);
				}

				renderPopMatrix();
				renderEnable(RenderCapability::Lighting);
				renderEnable(RenderCapability::DepthTest);
				RenderHelper::enableStandardItemLighting();
			}
		}
	}
}

bool GuiMerchant::isPointInRegion(int_t x, int_t y, int_t w, int_t h, int_t pointX, int_t pointY)
{
	int_t guiX = (width - xSize) / 2;
	int_t guiY = (height - ySize) / 2;
	pointX -= guiX;
	pointY -= guiY;
	return pointX >= x - 1 && pointX < x + w + 1 && pointY >= y - 1 && pointY < y + h + 1;
}

void GuiMerchant::drawItemStackTooltip(ItemStack *stack, int_t mouseX, int_t mouseY)
{
	if (stack == nullptr || fontRenderer == nullptr)
		return;
	std::vector<std::string> information = stack->getItemNameandInformation();
	if (information.empty())
		return;

	// Drawn over the 3D item icons: without depth test, as LegacyTradingScreen does.
	renderDisable(RenderCapability::DepthTest);
	renderDisable(RenderCapability::Lighting);

	int_t tooltipWidth = 0;
	for (const std::string &line : information)
		tooltipWidth = std::max(tooltipWidth, fontRenderer->getStringWidth(line));

	int_t tooltipX = mouseX + 12;
	int_t tooltipY = mouseY - 12;
	if (tooltipX + tooltipWidth + 6 > width)
		tooltipX = width - tooltipWidth - 6;
	if (tooltipY + 24 > height)
		tooltipY = height - 24;
	if (tooltipY < 0)
		tooltipY = 0;

	int_t tooltipHeight = 8;
	if (information.size() > 1)
		tooltipHeight += 2 + (static_cast<int_t>(information.size()) - 1) * 10;

	const int_t background = static_cast<int_t>(0xf0100010u);
	drawGradientRect(tooltipX - 3, tooltipY - 4, tooltipX + tooltipWidth + 3, tooltipY - 3, background, background);
	drawGradientRect(tooltipX - 3, tooltipY + tooltipHeight + 3, tooltipX + tooltipWidth + 3, tooltipY + tooltipHeight + 4, background, background);
	drawGradientRect(tooltipX - 3, tooltipY - 3, tooltipX + tooltipWidth + 3, tooltipY + tooltipHeight + 3, background, background);
	drawGradientRect(tooltipX - 4, tooltipY - 3, tooltipX - 3, tooltipY + tooltipHeight + 3, background, background);
	drawGradientRect(tooltipX + tooltipWidth + 3, tooltipY - 3, tooltipX + tooltipWidth + 4, tooltipY + tooltipHeight + 3, background, background);

	const int_t borderTop = 0x505000ff;
	const int_t borderBottom = (borderTop & 0x00fefefe) >> 1 | (borderTop & static_cast<int_t>(0xff000000u));
	drawGradientRect(tooltipX - 3, tooltipY - 2, tooltipX - 2, tooltipY + tooltipHeight + 2, borderTop, borderBottom);
	drawGradientRect(tooltipX + tooltipWidth + 2, tooltipY - 2, tooltipX + tooltipWidth + 3, tooltipY + tooltipHeight + 2, borderTop, borderBottom);
	drawGradientRect(tooltipX - 3, tooltipY - 3, tooltipX + tooltipWidth + 3, tooltipY - 2, borderTop, borderTop);
	drawGradientRect(tooltipX - 3, tooltipY + tooltipHeight + 2, tooltipX + tooltipWidth + 3, tooltipY + tooltipHeight + 3, borderBottom, borderBottom);

	const char *hex = "0123456789abcdef";
	for (std::size_t i = 0; i < information.size(); ++i)
	{
		std::string line = information[i];
		if (i == 0)
		{
			int_t color = stack->getRarity().nameColor & 15;
			line = std::string(u8"§") + hex[color] + line;
		}
		else
		{
			line = std::string(u8"§7") + line;
		}

		fontRenderer->drawStringWithShadow(line, tooltipX, tooltipY, -1);
		if (i == 0)
			tooltipY += 2;
		tooltipY += 10;
	}
	renderEnable(RenderCapability::DepthTest);
}

IMerchant *GuiMerchant::getIMerchant() const
{
	return theIMerchant;
}

void GuiMerchant::detachMerchant()
{
	theIMerchant = nullptr;
	if (auto *container = dynamic_cast<ContainerMerchant *>(inventorySlots))
		container->detachMerchant();
}

void merchantScreensForget(IMerchant *merchant)
{
	Minecraft *mc = Minecraft::getMinecraft();
	if (mc == nullptr || merchant == nullptr)
		return;
	for (int idx = 0; idx < 2; ++idx)
	{
		GuiScreen *screen = mc->getPlayerScreen(idx);
		if (screen == nullptr && idx == 0)
			screen = mc->currentScreen;
		if (auto *gui = dynamic_cast<GuiMerchant *>(screen))
		{
			if (gui->getIMerchant() == merchant)
				gui->detachMerchant();
		}
		else if (auto *legacy = dynamic_cast<LegacyTradingScreen *>(screen))
		{
			if (legacy->getMerchant() == merchant)
				legacy->detachMerchant();
		}
	}
}
