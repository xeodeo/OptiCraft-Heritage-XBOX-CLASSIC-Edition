#include "GuiFlatPresets.h"

#include <algorithm>

#include "BiomeGenBase.h"
#include "Block.h"
#include "BlockGrass.h"
#include "BlockTallGrass.h"
#include "FlatGeneratorInfo.h"
#include "FontRenderer.h"
#include "GameSettings.h"
#include "GuiButton.h"
#include "GuiCreateFlatWorld.h"
#include "GuiTextField.h"
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
#include "pc/lwjgl/Keyboard.h"
#include "platform/Input.h"
#include "platform/PlatformConfig.h"
#include "platform/RenderAPI.h"

namespace
{
RenderItem &presetItemRenderer()
{
	static RenderItem s_renderer;
	return s_renderer;
}

std::string trOr(const char *key, const char *fallback)
{
	const std::string s = StatCollector::translateToLocal(key);
	return (s.empty() || s == key) ? std::string(fallback) : s;
}

void drawPanelTitle(FontRenderer *font, const std::string &text, int_t centerX, int_t y)
{
	if (font == nullptr)
		return;
	const int_t x = centerX - font->getStringWidth(text) / 2;
	font->drawString(text, x + 1, y + 1, 0xd0d0d0);
	font->drawString(text, x, y, 0x303030);
}

FlatPresetItem makePreset(const std::string &name, int_t iconId, int_t iconDamage, BiomeGenBase *biome,
                          const std::vector<std::string> &features,
                          const std::vector<FlatLayerInfo> &topDownLayers)
{
	FlatGeneratorInfo info;
	for (int_t i = static_cast<int_t>(topDownLayers.size()) - 1; i >= 0; --i)
		info.getFlatLayers().push_back(topDownLayers[static_cast<std::size_t>(i)]);
	info.setBiome(biome != nullptr ? biome->biomeID : 1);
	info.func_82645_d();
	for (const std::string &feat : features)
		info.getWorldFeatures()[feat] = FlatGeneratorInfo::FeatureParams();
	return FlatPresetItem{iconId, iconDamage, name, info.toString()};
}

constexpr int_t kPresetRowH = 20;
} // namespace

const std::vector<FlatPresetItem> &GuiFlatPresets::getPresets()
{
	static std::vector<FlatPresetItem> s_presets;
	if (!s_presets.empty())
		return s_presets;

	const int_t grassId = Block::grass != nullptr ? Block::grass->blockID : 2;
	const int_t dirtId = Block::dirt != nullptr ? Block::dirt->blockID : 3;
	const int_t cobbleId = Block::cobblestone != nullptr ? Block::cobblestone->blockID : 4;
	const int_t bedrockId = Block::bedrock != nullptr ? Block::bedrock->blockID : 7;
	const int_t stoneId = Block::stone != nullptr ? Block::stone->blockID : 1;
	const int_t waterStillId = Block::waterStill != nullptr ? Block::waterStill->blockID : 9;
	const int_t waterIconId = Item::bucketWater != nullptr ? Item::bucketWater->shiftedIndex : waterStillId;
	const int_t sandId = Block::sand != nullptr ? Block::sand->blockID : 12;
	const int_t sandStoneId = Block::sandStone != nullptr ? Block::sandStone->blockID : 24;
	const int_t tallGrassId = Block::tallGrass != nullptr ? Block::tallGrass->blockID : 31;
	const int_t snowId = Block::snow != nullptr ? Block::snow->blockID : 78;
	const int_t featherId = Item::feather != nullptr ? Item::feather->shiftedIndex : 288;
	const int_t redstoneId = Item::redstone != nullptr ? Item::redstone->shiftedIndex : 331;

	s_presets.push_back(makePreset(
		"Classic Flat", grassId, 0, BiomeGenBase::plains, {"village"},
		{FlatLayerInfo(1, grassId), FlatLayerInfo(2, dirtId), FlatLayerInfo(1, bedrockId)}));

	s_presets.push_back(makePreset(
		"Tunnelers' Dream", stoneId, 0, BiomeGenBase::extremeHills,
		{"biome_1", "dungeon", "decoration", "stronghold", "mineshaft"},
		{FlatLayerInfo(1, grassId), FlatLayerInfo(5, dirtId), FlatLayerInfo(230, stoneId), FlatLayerInfo(1, bedrockId)}));

	s_presets.push_back(makePreset(
		"Water World", waterIconId, 0, BiomeGenBase::plains,
		{"village", "biome_1"},
		{FlatLayerInfo(90, waterStillId), FlatLayerInfo(5, sandId), FlatLayerInfo(5, dirtId), FlatLayerInfo(5, stoneId), FlatLayerInfo(1, bedrockId)}));

	s_presets.push_back(makePreset(
		"Overworld", tallGrassId, 1, BiomeGenBase::plains,
		{"village", "biome_1", "decoration", "stronghold", "mineshaft", "dungeon", "lake", "lava_lake"},
		{FlatLayerInfo(1, grassId), FlatLayerInfo(3, dirtId), FlatLayerInfo(59, stoneId), FlatLayerInfo(1, bedrockId)}));

	s_presets.push_back(makePreset(
		"Snowy Kingdom", snowId, 0, BiomeGenBase::icePlains,
		{"village", "biome_1"},
		{FlatLayerInfo(1, snowId), FlatLayerInfo(1, grassId), FlatLayerInfo(3, dirtId), FlatLayerInfo(59, stoneId), FlatLayerInfo(1, bedrockId)}));

	s_presets.push_back(makePreset(
		"Bottomless Pit", featherId, 0, BiomeGenBase::plains,
		{"village", "biome_1"},
		{FlatLayerInfo(1, grassId), FlatLayerInfo(3, dirtId), FlatLayerInfo(2, cobbleId)}));

	s_presets.push_back(makePreset(
		"Desert", sandId, 0, BiomeGenBase::desert,
		{"village", "biome_1", "decoration", "stronghold", "mineshaft", "dungeon"},
		{FlatLayerInfo(8, sandId), FlatLayerInfo(52, sandStoneId), FlatLayerInfo(3, stoneId), FlatLayerInfo(1, bedrockId)}));

	s_presets.push_back(makePreset(
		"Redstone Ready", redstoneId, 0, BiomeGenBase::desert,
		{},
		{FlatLayerInfo(52, sandStoneId), FlatLayerInfo(3, stoneId), FlatLayerInfo(1, bedrockId)}));

	return s_presets;
}

GuiFlatPresets::GuiFlatPresets(GuiCreateFlatWorld *parent)
	: createFlatWorldGui(parent)
	, presetTextField(nullptr)
	, buttonUsePreset(nullptr)
	, buttonCancel(nullptr)
	, selectedPreset(0)
	, scrollOffset(0)
	, focusZone(1)
	, panoramaAvailable(false)
{
}

GuiFlatPresets::~GuiFlatPresets()
{
	if (presetTextField != nullptr)
		presetTextField->setFocused(false);
	delete presetTextField;
	presetTextField = nullptr;
}

void GuiFlatPresets::computeRects(int_t &fieldX, int_t &fieldY, int_t &fieldW,
                                  int_t &listLeft, int_t &listTop, int_t &listRight, int_t &listBottom) const
{
	const bool legacy = (mc != nullptr && mc->gameSettings != nullptr && mc->gameSettings->legacyUI);
	if (legacy)
	{
		fieldX = legacyLayout.contentX;
		fieldW = legacyLayout.contentWidth;
		fieldY = legacyLayout.panelY + 28;
		listLeft = legacyLayout.contentX;
		listRight = legacyLayout.contentX + legacyLayout.contentWidth;
		listTop = fieldY + 32;
		listBottom = legacyLayout.panelY + legacyLayout.panelHeight - 30;
	}
	else
	{
		fieldX = std::max<int_t>(16, std::min<int_t>(50, width / 10));
		fieldW = width - fieldX * 2;
		fieldY = 38;
		listLeft = fieldX;
		listRight = fieldX + fieldW;
		listTop = 76;
		listBottom = height - 36;
	}
	if (listBottom < listTop + kPresetRowH)
		listBottom = listTop + kPresetRowH;
}

void GuiFlatPresets::initGui()
{
	controlList.clear();
	lwjgl::Keyboard::enableRepeatEvents(true);

	titleText = trOr("createWorld.customize.presets.title", "Select a Preset");
	shareText = trOr("createWorld.customize.presets.share", "Want to share your preset with someone? Use the below box!");
	listText = trOr("createWorld.customize.presets.list", "Alternatively, here's some we made earlier!");

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
	}

	int_t fieldX, fieldY, fieldW, listLeft, listTop, listRight, listBottom;
	computeRects(fieldX, fieldY, fieldW, listLeft, listTop, listRight, listBottom);

	std::string currentStr = (presetTextField != nullptr)
		? static_cast<std::string>(presetTextField->getText())
		: (createFlatWorldGui != nullptr ? createFlatWorldGui->getFlatGeneratorInfo() : "");
	if (presetTextField != nullptr)
		presetTextField->setFocused(false);
	delete presetTextField;
	presetTextField = new GuiTextField(this, fontRenderer, fieldX, fieldY, fieldW, 18, currentStr);
	presetTextField->setMaxStringLength(1230);
	presetTextField->setText(currentStr);

	if (legacy)
	{
		const int_t panelBottom = legacyLayout.panelY + legacyLayout.panelHeight;
		const int_t btnGap = 6;
		const int_t btnW = (legacyLayout.contentWidth - btnGap) / 2;
		const int_t btnY = panelBottom - 25;
		buttonUsePreset = new LegacyGuiButton(0, legacyLayout.contentX, btnY, btnW, 20,
			trOr("createWorld.customize.presets.select", "Use Preset"));
		buttonCancel = new LegacyGuiButton(1, legacyLayout.contentX + btnW + btnGap, btnY, btnW, 20,
			trOr("gui.cancel", "Cancel"));
	}
	else
	{
		const int_t btnW = std::min<int_t>(150, (width - 28) / 2);
		buttonUsePreset = new GuiButton(0, width / 2 - btnW - 5, height - 28, btnW, 20,
			trOr("createWorld.customize.presets.select", "Use Preset"));
		buttonCancel = new GuiButton(1, width / 2 + 5, height - 28, btnW, 20,
			trOr("gui.cancel", "Cancel"));
	}

	controlList.push_back(buttonUsePreset);
	controlList.push_back(buttonCancel);

	// Highlight matching preset if current string matches one
	const auto &presets = getPresets();
	for (int_t i = 0; i < static_cast<int_t>(presets.size()); ++i)
	{
		if (presets[static_cast<std::size_t>(i)].presetString == currentStr)
		{
			selectedPreset = i;
			break;
		}
	}

	updateButtonStates();
	ensureSelectedPresetVisible();
	syncFocus();
}

void GuiFlatPresets::onGuiClosed()
{
	lwjgl::Keyboard::enableRepeatEvents(false);
	if (presetTextField != nullptr)
		presetTextField->setFocused(false);
}

bool GuiFlatPresets::canUsePreset() const
{
	const auto &presets = getPresets();
	if (selectedPreset >= 0 && selectedPreset < static_cast<int_t>(presets.size()))
		return true;
	return presetTextField != nullptr && presetTextField->getText().length() > 1;
}

void GuiFlatPresets::updateButtonStates()
{
	if (buttonUsePreset != nullptr)
		buttonUsePreset->enabled = canUsePreset();
}

void GuiFlatPresets::syncFocus()
{
	if (buttonUsePreset != nullptr)
		buttonUsePreset->setKeyboardSelected(focusZone == 2 && buttonUsePreset->enabled);
	if (buttonCancel != nullptr)
		buttonCancel->setKeyboardSelected(focusZone == 3);
}

void GuiFlatPresets::selectPresetIndex(int_t index)
{
	const auto &presets = getPresets();
	if (index < 0 || index >= static_cast<int_t>(presets.size()))
		return;
	selectedPreset = index;
	if (presetTextField != nullptr)
		presetTextField->setText(presets[static_cast<std::size_t>(index)].presetString);
	updateButtonStates();
	ensureSelectedPresetVisible();
}

void GuiFlatPresets::ensureSelectedPresetVisible()
{
	int_t fx, fy, fw, l, t, r, b;
	computeRects(fx, fy, fw, l, t, r, b);
	const int_t visibleRows = std::max<int_t>(1, (b - t - 4) / kPresetRowH);
	const int_t count = static_cast<int_t>(getPresets().size());
	if (selectedPreset >= 0)
	{
		if (selectedPreset < scrollOffset)
			scrollOffset = selectedPreset;
		if (selectedPreset >= scrollOffset + visibleRows)
			scrollOffset = selectedPreset - visibleRows + 1;
	}
	const int_t maxScroll = std::max<int_t>(0, count - visibleRows);
	scrollOffset = std::max<int_t>(0, std::min<int_t>(scrollOffset, maxScroll));
}

void GuiFlatPresets::actionPerformed(GuiButton *button)
{
	if (button == nullptr || !button->enabled)
		return;

	if (button->id == 0 && canUsePreset())
	{
		if (createFlatWorldGui != nullptr && presetTextField != nullptr)
			createFlatWorldGui->setFlatGeneratorInfo(presetTextField->getText());
		mc->displayGuiScreen(createFlatWorldGui);
	}
	else if (button->id == 1)
	{
		mc->displayGuiScreen(createFlatWorldGui);
	}
}

void GuiFlatPresets::updateScreen()
{
	if (presetTextField != nullptr)
		presetTextField->updateCursorCounter();

#if PLATFORM_PS2 || PLATFORM_WII || PLATFORM_XBOX
	if (platformTextInputExclusive())
		return;
	const PlatformTextInputSnapshot pad = platformTextInputSnapshot(platformMenuPad());
	const int_t count = static_cast<int_t>(getPresets().size());
	const bool legacy = (mc != nullptr && mc->gameSettings != nullptr && mc->gameSettings->legacyUI);
	const char *focusSound = legacy ? "random.focus" : "random.click";
	const char *actSound = legacy ? "random.action" : "random.click";
	const char *backSound = legacy ? "random.back" : "random.click";

	if ((pad.pressed & (PLATFORM_TEXT_CLOSE | PLATFORM_TEXT_BACK)) != 0)
	{
		if (mc->sndManager != nullptr)
			mc->sndManager->playSoundFX(backSound, 1.0f, 1.0f);
		mc->displayGuiScreen(createFlatWorldGui);
		return;
	}

	if ((pad.pressed & PLATFORM_TEXT_SHIFT) != 0)
	{
		// Y button opens the on-screen keyboard directly on the preset string
		focusZone = 0;
		syncFocus();
		if (presetTextField != nullptr)
			presetTextField->setFocused(true);
		return;
	}

	if ((pad.pressed & PLATFORM_TEXT_UP) != 0)
	{
		if (focusZone == 1)
		{
			if (selectedPreset > 0)
				selectPresetIndex(selectedPreset - 1);
			else
				focusZone = 0;
		}
		else if (focusZone == 2 || focusZone == 3)
		{
			focusZone = 1;
			if (selectedPreset < 0 || selectedPreset >= count)
				selectPresetIndex(count - 1);
		}
		if (mc->sndManager != nullptr)
			mc->sndManager->playSoundFX(focusSound, 1.0f, 1.0f);
		syncFocus();
	}
	else if ((pad.pressed & PLATFORM_TEXT_DOWN) != 0)
	{
		if (focusZone == 0)
		{
			focusZone = 1;
			if (selectedPreset < 0)
				selectPresetIndex(0);
		}
		else if (focusZone == 1)
		{
			if (selectedPreset + 1 < count)
				selectPresetIndex(selectedPreset + 1);
			else
				focusZone = 2;
		}
		if (mc->sndManager != nullptr)
			mc->sndManager->playSoundFX(focusSound, 1.0f, 1.0f);
		syncFocus();
	}
	else if ((pad.pressed & (PLATFORM_TEXT_LEFT | PLATFORM_TEXT_RIGHT)) != 0)
	{
		if (focusZone == 2)
			focusZone = 3;
		else if (focusZone == 3)
			focusZone = 2;
		else
			focusZone = 2;
		if (mc->sndManager != nullptr)
			mc->sndManager->playSoundFX(focusSound, 1.0f, 1.0f);
		syncFocus();
	}
	else if ((pad.pressed & PLATFORM_TEXT_TYPE) != 0)
	{
		if (focusZone == 0)
		{
			if (presetTextField != nullptr)
				presetTextField->setFocused(true);
		}
		else if (focusZone == 1)
		{
			if (mc->sndManager != nullptr)
				mc->sndManager->playSoundFX(actSound, 1.0f, 1.0f);
			actionPerformed(buttonUsePreset);
		}
		else if (focusZone == 2)
		{
			if (mc->sndManager != nullptr)
				mc->sndManager->playSoundFX(actSound, 1.0f, 1.0f);
			actionPerformed(buttonUsePreset);
		}
		else if (focusZone == 3)
		{
			if (mc->sndManager != nullptr)
				mc->sndManager->playSoundFX(backSound, 1.0f, 1.0f);
			actionPerformed(buttonCancel);
		}
	}
#endif
}

void GuiFlatPresets::keyTyped(char_t c, int_t key)
{
	if (presetTextField != nullptr && presetTextField->getFocused())
	{
		if (key == 1 || key == 28 || c == '\r')
		{
			presetTextField->setFocused(false);
			updateButtonStates();
			syncFocus();
			return;
		}
		if (presetTextField->textboxKeyTyped(c, key))
		{
			updateButtonStates();
			return;
		}
	}

	if (key == 1)
	{
		mc->displayGuiScreen(createFlatWorldGui);
		return;
	}
	const int_t count = static_cast<int_t>(getPresets().size());
	if (key == 200)
	{
		if (focusZone == 1 && selectedPreset > 0)
			selectPresetIndex(selectedPreset - 1);
		else if (focusZone == 1)
			focusZone = 0;
		else if (focusZone >= 2)
			focusZone = 1;
		syncFocus();
		return;
	}
	if (key == 208)
	{
		if (focusZone == 0)
			focusZone = 1;
		else if (focusZone == 1 && selectedPreset + 1 < count)
			selectPresetIndex(selectedPreset + 1);
		else if (focusZone == 1)
			focusZone = 2;
		syncFocus();
		return;
	}
	if (key == 203 || key == 205)
	{
		focusZone = (focusZone == 2) ? 3 : 2;
		syncFocus();
		return;
	}
	if (key == 28 || c == '\r')
	{
		if (focusZone == 0 && presetTextField != nullptr)
			presetTextField->setFocused(true);
		else if (focusZone == 3)
			actionPerformed(buttonCancel);
		else
			actionPerformed(buttonUsePreset);
		return;
	}
	GuiScreen::keyTyped(c, key);
}

void GuiFlatPresets::mouseClicked(int_t mouseX, int_t mouseY, int_t button)
{
	if (presetTextField != nullptr)
		presetTextField->mouseClicked(mouseX, mouseY, button);
	if (button == 0)
	{
		int_t fx, fy, fw, l, t, r, b;
		computeRects(fx, fy, fw, l, t, r, b);
		if (mouseX >= fx && mouseX <= fx + fw && mouseY >= fy && mouseY <= fy + 18)
		{
			focusZone = 0;
			syncFocus();
			return;
		}
		if (mouseX >= l && mouseX <= r && mouseY >= t && mouseY <= b)
		{
			const int_t relRow = (mouseY - (t + 2)) / kPresetRowH;
			const int_t idx = scrollOffset + relRow;
			if (idx >= 0 && idx < static_cast<int_t>(getPresets().size()))
			{
				focusZone = 1;
				selectPresetIndex(idx);
				syncFocus();
				return;
			}
		}
	}
	GuiScreen::mouseClicked(mouseX, mouseY, button);
}

void GuiFlatPresets::drawPresetIcon(int_t x, int_t y, int_t iconId, int_t iconDamage)
{
	drawRect(x - 1, y - 1, x + 17, y + 17, 0xff8b8b8b);
	drawRect(x, y, x + 16, y + 16, 0xff373737);
	if (ItemStack::isValidItemID(iconId) &&
	    mc != nullptr && mc->renderEngine != nullptr)
	{
		ItemStack stack(iconId, 1, iconDamage);
		renderEnable(RenderCapability::RescaleNormal);
		RenderHelper::enableGUIStandardItemLighting();
		presetItemRenderer().renderItemIntoGUI(fontRenderer, mc->renderEngine, &stack, x, y);
		RenderHelper::disableStandardItemLighting();
		renderDisable(RenderCapability::RescaleNormal);
	}
}

void GuiFlatPresets::drawScreen(int_t mouseX, int_t mouseY, float_t partialTick)
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
		drawPanelTitle(fontRenderer, titleText, width / 2, legacyLayout.panelY + 5);
	}
	else
	{
		drawDefaultBackground();
		drawCenteredString(fontRenderer, titleText, width / 2, 8, 0xffffff);
	}

	int_t fx, fy, fw, l, t, r, b;
	computeRects(fx, fy, fw, l, t, r, b);

	const int_t labelColor = legacy ? 0x383838 : 0xa0a0a0;
	const std::string shareTrim = (fontRenderer->getStringWidth(shareText) > fw)
		? (fontRenderer->trimStringToWidth(shareText, fw - 6) + "...") : shareText;
	const std::string listTrim = (fontRenderer->getStringWidth(listText) > fw)
		? (fontRenderer->trimStringToWidth(listText, fw - 6) + "...") : listText;

	if (legacy)
	{
		fontRenderer->drawString(shareTrim, fx, fy - 10, labelColor);
		fontRenderer->drawString(listTrim, l, t - 10, labelColor);
	}
	else
	{
		drawString(fontRenderer, shareTrim, fx, fy - 10, labelColor);
		drawString(fontRenderer, listTrim, l, t - 10, labelColor);
	}

	if (focusZone == 0 && presetTextField != nullptr && !presetTextField->getFocused())
		drawRect(fx - 2, fy - 2, fx + fw + 2, fy + 20, 0xffffff66);
	if (presetTextField != nullptr)
		presetTextField->drawTextBox();

	// List frame
	drawRect(l - 1, t - 1, r + 1, b + 1, legacy ? 0xff404040 : 0xff808080);
	drawRect(l, t, r, b, 0xc0101010);

	const auto &presets = getPresets();
	const int_t count = static_cast<int_t>(presets.size());
	const int_t visibleRows = std::max<int_t>(1, (b - t - 4) / kPresetRowH);
	const int_t maxScroll = std::max<int_t>(0, count - visibleRows);
	if (scrollOffset > maxScroll)
		scrollOffset = maxScroll;

	for (int_t row = 0; row < visibleRows; ++row)
	{
		const int_t idx = scrollOffset + row;
		if (idx >= count)
			break;
		const FlatPresetItem &item = presets[static_cast<std::size_t>(idx)];
		const int_t rowY = t + 2 + row * kPresetRowH;
		const bool isSel = (idx == selectedPreset);

		if (isSel)
		{
			const int_t borderCol = (focusZone == 1) ? 0xffffff66 : 0xff808080;
			drawRect(l + 2, rowY, r - 6, rowY + kPresetRowH - 2, borderCol);
			drawRect(l + 3, rowY + 1, r - 7, rowY + kPresetRowH - 3, 0xff222222);
		}

		drawPresetIcon(l + 6, rowY + 1, item.iconId, item.iconDamage);
		drawString(fontRenderer, item.name, l + 28, rowY + 5, isSel ? 0xffffa0 : 0xffffff);
	}

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
