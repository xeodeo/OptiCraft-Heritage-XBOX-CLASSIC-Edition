#include "GuiCreateFlatWorld.h"

#include <algorithm>

#include "Block.h"
#include "FontRenderer.h"
#include "GameSettings.h"
#include "GuiButton.h"
#include "GuiCreateWorld.h"
#include "GuiFlatPresets.h"
#include "Item.h"
#include "ItemStack.h"
#include "Minecraft.h"
#include "RenderEngine.h"
#include "RenderHelper.h"
#include "RenderItem.h"
#include "SoundManager.h"
#include "StatCollector.h"
#include "legacy/LegacyGuiButton.h"
#include "legacy/LegacyMainMenuLayout.h"
#include "legacy/LegacyMenuHints.h"
#include "legacy/LegacyPanorama.h"
#include "legacy/LegacySceneState.h"
#include "legacy/LegacyUiAssets.h"
#include "platform/Input.h"
#include "platform/PlatformConfig.h"
#include "platform/RenderAPI.h"

namespace
{
RenderItem &flatLayerItemRenderer()
{
	static RenderItem s_renderer;
	return s_renderer;
}

std::string trOr(const char *key, const char *fallback)
{
	const std::string s = StatCollector::translateToLocal(key);
	return (s.empty() || s == key) ? std::string(fallback) : s;
}

std::string layerBlockDisplayName(int_t blockId, int_t blockMeta)
{
	if (blockId <= 0 || blockId >= Block::BLOCK_REGISTRY_SIZE || Block::blocksList[blockId] == nullptr)
		return trOr("tile.air.name", "Air");

	if (Item::itemsList[blockId] != nullptr)
	{
		ItemStack stack(blockId, 1, blockMeta);
		const std::string itemName = stack.getDisplayName();
		if (!itemName.empty())
			return itemName;
	}
	const std::string blockName = Block::blocksList[blockId]->translateBlockName();
	if (!blockName.empty())
		return blockName;
	return "Block " + std::to_string(blockId);
}

int_t iconItemIdForBlock(int_t blockId)
{
	if (blockId <= 0)
		return -1;
	if (Block::waterStill != nullptr && (blockId == Block::waterStill->blockID || (Block::waterMoving != nullptr && blockId == Block::waterMoving->blockID)))
		return Item::bucketWater != nullptr ? Item::bucketWater->shiftedIndex : -1;
	if (Block::lavaStill != nullptr && (blockId == Block::lavaStill->blockID || (Block::lavaMoving != nullptr && blockId == Block::lavaMoving->blockID)))
		return Item::bucketLava != nullptr ? Item::bucketLava->shiftedIndex : -1;
	if (ItemStack::isValidItemID(blockId))
		return blockId;
	return -1;
}

void drawPanelTitle(FontRenderer *font, const std::string &text, int_t centerX, int_t y)
{
	if (font == nullptr)
		return;
	const int_t x = centerX - font->getStringWidth(text) / 2;
	font->drawString(text, x + 1, y + 1, 0xd0d0d0);
	font->drawString(text, x, y, 0x303030);
}

constexpr int_t kLayerRowH = 22;
} // namespace

GuiCreateFlatWorld::GuiCreateFlatWorld(GuiCreateWorld *parent, const std::string &generatorOptions)
	: createWorldGui(parent)
	, theFlatGeneratorInfo(FlatGeneratorInfo::createFlatGeneratorFromString(generatorOptions))
	, buttonRemoveLayer(nullptr)
	, buttonPresets(nullptr)
	, buttonDone(nullptr)
	, buttonCancel(nullptr)
	, selectedLayer(0)
	, scrollOffset(0)
	, focusOnList(true)
	, selectedButton(1)
	, panoramaAvailable(false)
{
}

std::string GuiCreateFlatWorld::getFlatGeneratorInfo() const
{
	return theFlatGeneratorInfo.toString();
}

void GuiCreateFlatWorld::setFlatGeneratorInfo(const std::string &options)
{
	theFlatGeneratorInfo = FlatGeneratorInfo::createFlatGeneratorFromString(options);
	selectedLayer = theFlatGeneratorInfo.getFlatLayers().empty() ? -1 : 0;
	scrollOffset = 0;
}

void GuiCreateFlatWorld::computeListRect(int_t &left, int_t &top, int_t &right, int_t &bottom) const
{
	const bool legacy = (mc != nullptr && mc->gameSettings != nullptr && mc->gameSettings->legacyUI);
	if (legacy)
	{
		left = legacyLayout.contentX;
		right = legacyLayout.contentX + legacyLayout.contentWidth;
		top = legacyLayout.panelY + 30;
		bottom = legacyLayout.panelY + legacyLayout.panelHeight - 54;
	}
	else
	{
		const int_t listW = std::min<int_t>(300, width - 24);
		left = (width - listW) / 2;
		right = left + listW;
		top = 44;
		bottom = height - 60;
	}
	if (bottom < top + kLayerRowH)
		bottom = top + kLayerRowH;
}

void GuiCreateFlatWorld::initGui()
{
	controlList.clear();
	customizationTitle = trOr("createWorld.customize.flat.title", "Superflat Customization");
	layerMaterialLabel = trOr("createWorld.customize.flat.tile", "Layer Material");
	heightLabel = trOr("createWorld.customize.flat.height", "Height");

	const bool legacy = (mc != nullptr && mc->gameSettings != nullptr && mc->gameSettings->legacyUI);
	panoramaAvailable = legacy && mc != nullptr && mc->renderEngine != nullptr &&
		mc->renderEngine->hasResource(legacyPanoramaResourcePath());

	if (legacy)
	{
		legacyLayout = legacyOptionsLayout(width, height, 6, LegacyOptionsLayoutPreset::Form);
		const int_t maxPanelW = std::min<int_t>(330, width - 24);
		if (legacyLayout.panelWidth < maxPanelW)
		{
			legacyLayout.panelWidth = maxPanelW;
			legacyLayout.panelX = (width - legacyLayout.panelWidth) / 2;
			legacyLayout.contentX = legacyLayout.panelX + 12;
			legacyLayout.contentWidth = legacyLayout.panelWidth - 24;
		}
		const int_t panelBottom = legacyLayout.panelY + legacyLayout.panelHeight;
		const int_t btnGap = 6;
		const int_t btnW = (legacyLayout.contentWidth - btnGap) / 2;
		const int_t btnH = 20;
		const int_t row1Y = panelBottom - 48;
		const int_t row2Y = panelBottom - 25;
		const int_t leftX = legacyLayout.contentX;
		const int_t rightX = leftX + btnW + btnGap;

		buttonRemoveLayer = new LegacyGuiButton(4, leftX, row1Y, btnW, btnH, trOr("createWorld.customize.flat.removeLayer", "Remove Layer"));
		buttonPresets = new LegacyGuiButton(5, rightX, row1Y, btnW, btnH, trOr("createWorld.customize.presets", "Presets"));
		buttonDone = new LegacyGuiButton(0, leftX, row2Y, btnW, btnH, trOr("gui.done", "Done"));
		buttonCancel = new LegacyGuiButton(1, rightX, row2Y, btnW, btnH, trOr("gui.cancel", "Cancel"));
	}
	else
	{
		const int_t btnW = std::min<int_t>(150, (width - 28) / 2);
		const int_t leftX = width / 2 - btnW - 5;
		const int_t rightX = width / 2 + 5;
		const int_t row1Y = height - 52;
		const int_t row2Y = height - 28;

		buttonRemoveLayer = new GuiButton(4, leftX, row1Y, btnW, 20, trOr("createWorld.customize.flat.removeLayer", "Remove Layer"));
		buttonPresets = new GuiButton(5, rightX, row1Y, btnW, 20, trOr("createWorld.customize.presets", "Presets"));
		buttonDone = new GuiButton(0, leftX, row2Y, btnW, 20, trOr("gui.done", "Done"));
		buttonCancel = new GuiButton(1, rightX, row2Y, btnW, 20, trOr("gui.cancel", "Cancel"));
	}

	controlList.push_back(buttonRemoveLayer);
	controlList.push_back(buttonPresets);
	controlList.push_back(buttonDone);
	controlList.push_back(buttonCancel);

	theFlatGeneratorInfo.func_82645_d();
	const int_t count = static_cast<int_t>(theFlatGeneratorInfo.getFlatLayers().size());
	if (count <= 0)
		selectedLayer = -1;
	else if (selectedLayer < 0 || selectedLayer >= count)
		selectedLayer = 0;

	updateButtonStates();
	ensureSelectedLayerVisible();
	syncButtonFocus();
}

bool GuiCreateFlatWorld::hasValidSelectedLayer() const
{
	return selectedLayer >= 0 && selectedLayer < static_cast<int_t>(theFlatGeneratorInfo.getFlatLayers().size());
}

void GuiCreateFlatWorld::updateButtonStates()
{
	if (buttonRemoveLayer != nullptr)
		buttonRemoveLayer->enabled = hasValidSelectedLayer();
}

void GuiCreateFlatWorld::syncButtonFocus()
{
	GuiButton *buttons[4] = {buttonRemoveLayer, buttonPresets, buttonDone, buttonCancel};
	for (int_t i = 0; i < 4; ++i)
	{
		if (buttons[i] != nullptr)
			buttons[i]->setKeyboardSelected(!focusOnList && selectedButton == i && buttons[i]->enabled);
	}
}

void GuiCreateFlatWorld::ensureSelectedLayerVisible()
{
	int_t l, t, r, b;
	computeListRect(l, t, r, b);
	const int_t visibleRows = std::max<int_t>(1, (b - t - 4) / kLayerRowH);
	const int_t count = static_cast<int_t>(theFlatGeneratorInfo.getFlatLayers().size());
	if (selectedLayer >= 0)
	{
		if (selectedLayer < scrollOffset)
			scrollOffset = selectedLayer;
		if (selectedLayer >= scrollOffset + visibleRows)
			scrollOffset = selectedLayer - visibleRows + 1;
	}
	const int_t maxScroll = std::max<int_t>(0, count - visibleRows);
	scrollOffset = std::max<int_t>(0, std::min<int_t>(scrollOffset, maxScroll));
}

void GuiCreateFlatWorld::actionPerformed(GuiButton *button)
{
	if (button == nullptr || !button->enabled)
		return;

	auto &layers = theFlatGeneratorInfo.getFlatLayers();
	if (button->id == 1)
	{
		mc->displayGuiScreen(createWorldGui);
		return;
	}
	if (button->id == 0)
	{
		if (createWorldGui != nullptr)
			createWorldGui->generatorOptionsToUse = getFlatGeneratorInfo();
		mc->displayGuiScreen(createWorldGui);
		return;
	}
	if (button->id == 5)
	{
		mc->displayGuiScreen(new GuiFlatPresets(this));
		return;
	}
	if (button->id == 4 && hasValidSelectedLayer())
	{
		const int_t layerIdx = static_cast<int_t>(layers.size()) - selectedLayer - 1;
		if (layerIdx >= 0 && layerIdx < static_cast<int_t>(layers.size()))
			layers.erase(layers.begin() + layerIdx);
		selectedLayer = std::min<int_t>(selectedLayer, static_cast<int_t>(layers.size()) - 1);
		if (layers.empty())
		{
			focusOnList = false;
			selectedButton = 1; // Presets
		}
	}

	theFlatGeneratorInfo.func_82645_d();
	updateButtonStates();
	ensureSelectedLayerVisible();
	syncButtonFocus();
}

void GuiCreateFlatWorld::updateScreen()
{
#if PLATFORM_PS2 || PLATFORM_WII || PLATFORM_XBOX
	if (platformTextInputExclusive())
		return;
	const PlatformTextInputSnapshot pad = platformTextInputSnapshot(platformMenuPad());
	const int_t count = static_cast<int_t>(theFlatGeneratorInfo.getFlatLayers().size());
	const bool legacy = (mc != nullptr && mc->gameSettings != nullptr && mc->gameSettings->legacyUI);
	const char *focusSound = legacy ? "random.focus" : "random.click";
	const char *actSound = legacy ? "random.action" : "random.click";
	const char *backSound = legacy ? "random.back" : "random.click";

	if ((pad.pressed & (PLATFORM_TEXT_CLOSE | PLATFORM_TEXT_BACK)) != 0)
	{
		if (mc->sndManager != nullptr)
			mc->sndManager->playSoundFX(backSound, 1.0f, 1.0f);
		mc->displayGuiScreen(createWorldGui);
		return;
	}

	if ((pad.pressed & PLATFORM_TEXT_SHIFT) != 0)
	{
		// Y button -> quick shortcut to Presets
		if (mc->sndManager != nullptr)
			mc->sndManager->playSoundFX(actSound, 1.0f, 1.0f);
		actionPerformed(buttonPresets);
		return;
	}

	if ((pad.pressed & PLATFORM_TEXT_UP) != 0)
	{
		if (focusOnList)
		{
			if (selectedLayer > 0)
			{
				--selectedLayer;
				ensureSelectedLayerVisible();
				if (mc->sndManager != nullptr)
					mc->sndManager->playSoundFX(focusSound, 1.0f, 1.0f);
			}
		}
		else if (selectedButton >= 2)
		{
			selectedButton -= 2;
			if (selectedButton == 0 && (buttonRemoveLayer == nullptr || !buttonRemoveLayer->enabled))
				selectedButton = 1;
			if (mc->sndManager != nullptr)
				mc->sndManager->playSoundFX(focusSound, 1.0f, 1.0f);
		}
		else if (count > 0)
		{
			focusOnList = true;
			if (selectedLayer < 0 || selectedLayer >= count)
				selectedLayer = count - 1;
			ensureSelectedLayerVisible();
			if (mc->sndManager != nullptr)
				mc->sndManager->playSoundFX(focusSound, 1.0f, 1.0f);
		}
		syncButtonFocus();
	}
	else if ((pad.pressed & PLATFORM_TEXT_DOWN) != 0)
	{
		if (focusOnList)
		{
			if (selectedLayer + 1 < count)
			{
				++selectedLayer;
				ensureSelectedLayerVisible();
			}
			else
			{
				focusOnList = false;
				selectedButton = (buttonRemoveLayer != nullptr && buttonRemoveLayer->enabled) ? 0 : 1;
			}
			if (mc->sndManager != nullptr)
				mc->sndManager->playSoundFX(focusSound, 1.0f, 1.0f);
		}
		else if (selectedButton < 2)
		{
			selectedButton += 2;
			if (mc->sndManager != nullptr)
				mc->sndManager->playSoundFX(focusSound, 1.0f, 1.0f);
		}
		syncButtonFocus();
	}
	else if ((pad.pressed & (PLATFORM_TEXT_LEFT | PLATFORM_TEXT_RIGHT)) != 0)
	{
		if (focusOnList)
		{
			focusOnList = false;
			selectedButton = 1; // Presets
		}
		else
		{
			int_t toggled = selectedButton ^ 1;
			if (toggled == 0 && (buttonRemoveLayer == nullptr || !buttonRemoveLayer->enabled))
				toggled = 1;
			selectedButton = toggled;
		}
		if (mc->sndManager != nullptr)
			mc->sndManager->playSoundFX(focusSound, 1.0f, 1.0f);
		syncButtonFocus();
	}
	else if ((pad.pressed & PLATFORM_TEXT_TYPE) != 0)
	{
		if (focusOnList)
		{
			focusOnList = false;
			selectedButton = (buttonRemoveLayer != nullptr && buttonRemoveLayer->enabled) ? 0 : 1;
			if (mc->sndManager != nullptr)
				mc->sndManager->playSoundFX(focusSound, 1.0f, 1.0f);
			syncButtonFocus();
		}
		else
		{
			GuiButton *buttons[4] = {buttonRemoveLayer, buttonPresets, buttonDone, buttonCancel};
			GuiButton *target = buttons[selectedButton];
			if (target != nullptr && target->enabled)
			{
				if (mc->sndManager != nullptr)
					mc->sndManager->playSoundFX(actSound, 1.0f, 1.0f);
				actionPerformed(target);
			}
		}
	}
#endif
}

void GuiCreateFlatWorld::keyTyped(char_t c, int_t key)
{
	const int_t count = static_cast<int_t>(theFlatGeneratorInfo.getFlatLayers().size());
	if (key == 1)
	{
		mc->displayGuiScreen(createWorldGui);
		return;
	}
	if (key == 200) // Up
	{
		if (focusOnList && selectedLayer > 0)
		{
			--selectedLayer;
			ensureSelectedLayerVisible();
		}
		else if (!focusOnList && selectedButton >= 2)
		{
			selectedButton -= 2;
		}
		else if (!focusOnList && count > 0)
		{
			focusOnList = true;
			selectedLayer = count - 1;
			ensureSelectedLayerVisible();
		}
		syncButtonFocus();
		return;
	}
	if (key == 208) // Down
	{
		if (focusOnList)
		{
			if (selectedLayer + 1 < count)
			{
				++selectedLayer;
				ensureSelectedLayerVisible();
			}
			else
			{
				focusOnList = false;
				selectedButton = hasValidSelectedLayer() ? 0 : 1;
			}
		}
		else if (selectedButton < 2)
		{
			selectedButton += 2;
		}
		syncButtonFocus();
		return;
	}
	if (key == 203 || key == 205) // Left / Right
	{
		if (focusOnList)
		{
			focusOnList = false;
			selectedButton = 1;
		}
		else
		{
			selectedButton ^= 1;
			if (selectedButton == 0 && !hasValidSelectedLayer())
				selectedButton = 1;
		}
		syncButtonFocus();
		return;
	}
	if (key == 28 || c == '\r')
	{
		if (focusOnList)
		{
			focusOnList = false;
			selectedButton = 2; // Done
			syncButtonFocus();
		}
		else
		{
			GuiButton *buttons[4] = {buttonRemoveLayer, buttonPresets, buttonDone, buttonCancel};
			actionPerformed(buttons[selectedButton]);
		}
		return;
	}
	GuiScreen::keyTyped(c, key);
}

void GuiCreateFlatWorld::mouseClicked(int_t mouseX, int_t mouseY, int_t button)
{
	if (button == 0)
	{
		int_t l, t, r, b;
		computeListRect(l, t, r, b);
		if (mouseX >= l && mouseX <= r && mouseY >= t && mouseY <= b)
		{
			const int_t relRow = (mouseY - (t + 2)) / kLayerRowH;
			const int_t idx = scrollOffset + relRow;
			const int_t count = static_cast<int_t>(theFlatGeneratorInfo.getFlatLayers().size());
			if (idx >= 0 && idx < count)
			{
				selectedLayer = idx;
				focusOnList = true;
				updateButtonStates();
				ensureSelectedLayerVisible();
				syncButtonFocus();
				return;
			}
		}
	}
	GuiScreen::mouseClicked(mouseX, mouseY, button);
}

void GuiCreateFlatWorld::drawLayerIcon(int_t x, int_t y, int_t blockId, int_t blockMeta)
{
	// Frame matching Java GuiCreateFlatWorldListSlot
	drawRect(x - 1, y - 1, x + 17, y + 17, 0xff8b8b8b);
	drawRect(x, y, x + 16, y + 16, 0xff373737);

	const int_t itemId = iconItemIdForBlock(blockId);
	if (itemId >= 0 && mc != nullptr && mc->renderEngine != nullptr)
	{
		ItemStack stack(itemId, 1, blockMeta);
		renderEnable(RenderCapability::RescaleNormal);
		RenderHelper::enableGUIStandardItemLighting();
		flatLayerItemRenderer().renderItemIntoGUI(fontRenderer, mc->renderEngine, &stack, x, y);
		RenderHelper::disableStandardItemLighting();
		renderDisable(RenderCapability::RescaleNormal);
	}
}

void GuiCreateFlatWorld::drawScreen(int_t mouseX, int_t mouseY, float_t partialTick)
{
	const bool legacy = (mc != nullptr && mc->gameSettings != nullptr && mc->gameSettings->legacyUI);
	if (legacy)
	{
		const bool drewPanorama = panoramaAvailable &&
			legacyDrawPanorama(mc, width, height, legacyScenePanoramaTimer(), partialTick, zLevel);
		if (!drewPanorama)
			drawDefaultBackground();
		else
			drawGradientRect(0, 0, width, height, static_cast<int_t>(0x14000000u), static_cast<int_t>(0x3c000000u));

		const bool is480 = (mc != nullptr && mc->displayHeight <= 480);
		if (!is480)
		{
			LegacyMainMenuLayout titleLayout{};
			titleLayout.titleY = legacyLayout.titleY;
			titleLayout.titleMaxWidth = legacyLayout.titleMaxWidth;
			titleLayout.titleMaxHeight = legacyLayout.titleMaxHeight;
			legacyDrawTitleTexture(mc, titleLayout, width, zLevel, nullptr);
		}
		panelRenderer.draw(legacyLayout);
		drawPanelTitle(fontRenderer, customizationTitle, width / 2, legacyLayout.panelY + 5);
	}
	else
	{
		drawDefaultBackground();
		drawCenteredString(fontRenderer, customizationTitle, width / 2, 8, 0xffffff);
	}

	int_t l, t, r, b;
	computeListRect(l, t, r, b);

	// Column headers
	const int_t headerY = t - 11;
	const int_t headerColor = legacy ? 0x383838 : 0xffffff;
	if (legacy)
	{
		fontRenderer->drawString(layerMaterialLabel, l + 2, headerY, headerColor);
		fontRenderer->drawString(heightLabel, r - fontRenderer->getStringWidth(heightLabel) - 2, headerY, headerColor);
	}
	else
	{
		drawString(fontRenderer, layerMaterialLabel, l + 2, headerY, headerColor);
		drawString(fontRenderer, heightLabel, r - fontRenderer->getStringWidth(heightLabel) - 2, headerY, headerColor);
	}

	// List frame
	drawRect(l - 1, t - 1, r + 1, b + 1, legacy ? 0xff404040 : 0xff808080);
	drawRect(l, t, r, b, 0xc0101010);

	const auto &layers = theFlatGeneratorInfo.getFlatLayers();
	const int_t count = static_cast<int_t>(layers.size());
	const int_t visibleRows = std::max<int_t>(1, (b - t - 4) / kLayerRowH);
	const int_t maxScroll = std::max<int_t>(0, count - visibleRows);
	if (scrollOffset > maxScroll)
		scrollOffset = maxScroll;

	for (int_t row = 0; row < visibleRows; ++row)
	{
		const int_t displayIdx = scrollOffset + row;
		if (displayIdx >= count)
			break;

		const int_t layerIdx = count - displayIdx - 1;
		const FlatLayerInfo &layer = layers[static_cast<std::size_t>(layerIdx)];
		const int_t rowY = t + 2 + row * kLayerRowH;
		const bool isSel = (displayIdx == selectedLayer);

		if (isSel)
		{
			const int_t borderCol = focusOnList ? 0xffffff66 : 0xff808080;
			drawRect(l + 2, rowY, r - 6, rowY + kLayerRowH - 2, borderCol);
			drawRect(l + 3, rowY + 1, r - 7, rowY + kLayerRowH - 3, 0xff222222);
		}

		drawLayerIcon(l + 6, rowY + 2, layer.getFillBlock(), layer.getFillBlockMeta());

		std::string nameStr = layerBlockDisplayName(layer.getFillBlock(), layer.getFillBlockMeta());
		const std::string countStr = std::to_string(layer.getLayerCount());
		std::string heightStr;
		if (displayIdx == 0)
			heightStr = StatCollector::translateToLocalFormatted("createWorld.customize.flat.layer.top", countStr);
		else if (displayIdx == count - 1)
			heightStr = StatCollector::translateToLocalFormatted("createWorld.customize.flat.layer.bottom", countStr);
		else
			heightStr = StatCollector::translateToLocalFormatted("createWorld.customize.flat.layer", countStr);
		if (heightStr.find("createWorld.") == 0)
			heightStr = countStr;

		const int_t heightW = fontRenderer->getStringWidth(heightStr);
		const int_t maxNameW = std::max<int_t>(20, (r - 10 - heightW) - (l + 28));
		if (fontRenderer->getStringWidth(nameStr) > maxNameW)
			nameStr = fontRenderer->trimStringToWidth(nameStr, maxNameW - 6) + "...";

		drawString(fontRenderer, nameStr, l + 28, rowY + 6, isSel ? 0xffffa0 : 0xffffff);
		drawString(fontRenderer, heightStr, r - 10 - heightW, rowY + 6, 0xcccccc);
	}

	// Scrollbar indicator if needed
	if (count > visibleRows)
	{
		const int_t trackTop = t + 2;
		const int_t trackH = std::max<int_t>(10, b - t - 4);
		const int_t thumbH = std::max<int_t>(12, trackH * visibleRows / count);
		const int_t thumbY = trackTop + (maxScroll > 0 ? ((trackH - thumbH) * scrollOffset / maxScroll) : 0);
		drawRect(r - 4, trackTop, r - 1, trackTop + trackH, 0xff202020);
		drawRect(r - 4, thumbY, r - 1, thumbY + thumbH, 0xffc0c0c0);
	}

	GuiScreen::drawScreen(mouseX, mouseY, partialTick);
	if (legacy)
		drawLegacyMenuHints(mc, width, height, true);
}
