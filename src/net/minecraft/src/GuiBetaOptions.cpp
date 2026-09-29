#include "net/minecraft/src/UiStrings.h"
#include "GuiBetaOptions.h"

#include "EnumOptions.h"
#include "EntityRenderer.h"
#include "GameSettings.h"
#include "GuiButton.h"
#include "GuiSmallButton.h"
#include "GuiDeadzoneSettings.h"
#include "GuiTextField.h"
#include "Minecraft.h"
#include "ScaledResolution.h"
#include "Session.h"
#include "platform/PlatformConfig.h"
#include "platform/PlatformUserSettings.h"
#include "net/minecraft/src/legacy/LegacyUiPolicy.h"

namespace
{
constexpr int_t BUTTON_DONE = 200;
constexpr int_t BUTTON_ALTERNATIVE_CONTROLS = 201;
constexpr int_t BUTTON_DEADZONE = 202;
constexpr int_t BUTTON_ASPECT_RATIO = 203;
constexpr int_t BUTTON_LEGACY_UI = 204;
constexpr int_t BUTTON_LEGACY_LOOK = 205;
constexpr int_t BUTTON_LEGACY_CRAFTING = 207;
constexpr int_t BUTTON_LEGACY_CREATIVE = 208;
}

GuiBetaOptions::GuiBetaOptions(GuiScreen *parent, GameSettings *options)
	: parentScreen(parent), settings(options), nameField(nullptr)
{
}

GuiBetaOptions::~GuiBetaOptions()
{
	delete nameField;
}

void GuiBetaOptions::initGui()
{
	controlList.clear();
	delete nameField;

	int_t topY = height / 6 - 8;
	if (topY < 18) topY = 18;

	nameField = new GuiTextField(this, fontRenderer,
		width / 2 - 100, topY + 14, 200, 20, settings->playerName);
	nameField->setMaxStringLength(16);
	nameField->setFocused(false);

	int_t by = topY + 40;
	// Row 1: Legacy UI & Legacy Look
	controlList.push_back(new GuiSmallButton(BUTTON_LEGACY_UI, width / 2 - 155, by,
		legacyUiOptionLabel(settings->legacyUI)));
	controlList.push_back(new GuiSmallButton(BUTTON_LEGACY_LOOK, width / 2 + 5, by,
		uiText("Legacy Look: ") + std::string(settings->legacyLook ? uiText("ON") : uiText("OFF"))));
	by += 24;

	// Row 2: Legacy Crafting & Legacy Creative
	controlList.push_back(new GuiSmallButton(BUTTON_LEGACY_CRAFTING, width / 2 - 155, by,
		uiText("Legacy Crafting: ") + std::string(settings->legacyCrafting ? uiText("ON") : uiText("OFF"))));
	controlList.push_back(new GuiSmallButton(BUTTON_LEGACY_CREATIVE, width / 2 + 5, by,
		uiText("Legacy Creative: ") + std::string(settings->legacyCreative ? uiText("ON") : uiText("OFF"))));
	by += 24;

#if PLATFORM_HAS_ASPECT_RATIO_OPTION
	controlList.push_back(new GuiSmallButton(BUTTON_ASPECT_RATIO, width / 2 - 155, by,
		settings->getKeyBinding(EnumOptions::ASPECT_RATIO)));
#ifdef WII_PLATFORM
	controlList.push_back(new GuiSmallButton(BUTTON_ALTERNATIVE_CONTROLS, width / 2 + 5, by,
		uiText("Alt Controls: ") + std::string(settings->alternativeControllerLayout ? uiText("ON") : uiText("OFF"))));
	by += 24;
	controlList.push_back(new GuiButton(BUTTON_DEADZONE, width / 2 - 100, by, uiText("Deadzone Settings...")));
	by += 24;
#elif PLATFORM_HAS_CONTROLLER_CALIBRATION
	controlList.push_back(new GuiSmallButton(BUTTON_DEADZONE, width / 2 + 5, by, uiText("Deadzone Settings...")));
	by += 24;
#else
	by += 24;
#endif
#else
#ifdef WII_PLATFORM
	controlList.push_back(new GuiSmallButton(BUTTON_ALTERNATIVE_CONTROLS, width / 2 - 155, by,
		uiText("Alt Controls: ") + std::string(settings->alternativeControllerLayout ? uiText("ON") : uiText("OFF"))));
	controlList.push_back(new GuiSmallButton(BUTTON_DEADZONE, width / 2 + 5, by, uiText("Deadzone Settings...")));
	by += 24;
#elif PLATFORM_HAS_CONTROLLER_CALIBRATION
	controlList.push_back(new GuiButton(BUTTON_DEADZONE, width / 2 - 100, by, uiText("Deadzone Settings...")));
	by += 24;
#endif
#endif

	controlList.push_back(new GuiButton(BUTTON_DONE, width / 2 - 100, by + 4, uiText("Done")));
}

void GuiBetaOptions::updateScreen()
{
	if (nameField != nullptr)
		nameField->updateCursorCounter();
}

void GuiBetaOptions::onGuiClosed()
{
	if (nameField != nullptr)
		nameField->setFocused(false);
}

std::string GuiBetaOptions::sanitizeName(const std::string &name)
{
	std::string result;
	for (char c : name)
	{
		bool valid = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
			(c >= '0' && c <= '9') || c == '_';
		if (valid && result.size() < 16)
			result.push_back(c);
	}
	return result.empty() ? "Player" : result;
}

void GuiBetaOptions::saveAndClose()
{
	settings->playerName = sanitizeName(nameField != nullptr ? nameField->getText() : "");
	if (mc->session != nullptr)
		mc->session->username = settings->playerName;
	settings->saveOptions();
	mc->displayGuiScreen(parentScreen);
}

void GuiBetaOptions::keyTyped(char_t c, int_t key)
{
	if (c == '\r')
	{
		saveAndClose();
		return;
	}
	if (nameField != nullptr)
		nameField->textboxKeyTyped(c, key);
}

void GuiBetaOptions::mouseClicked(int_t x, int_t y, int_t button)
{
	GuiScreen::mouseClicked(x, y, button);
	if (nameField != nullptr)
		nameField->mouseClicked(x, y, button);
}

void GuiBetaOptions::actionPerformed(GuiButton *button)
{
	if (button == nullptr || !button->enabled)
		return;
#if PLATFORM_HAS_ASPECT_RATIO_OPTION
	if (button->id == BUTTON_ASPECT_RATIO)
	{
		settings->playerName = sanitizeName(nameField != nullptr ? nameField->getText() : "");
		if (mc->session != nullptr)
			mc->session->username = settings->playerName;
		settings->setOptionValue(EnumOptions::ASPECT_RATIO, 1);
		ScaledResolution sr(settings, mc->displayWidth, mc->displayHeight);
		setWorldAndResolution(mc, sr.getScaledWidth(), sr.getScaledHeight());
		return;
	}
#endif
	if (button->id == BUTTON_LEGACY_UI)
	{
		const int_t previousScale = settings->guiScale;
		settings->setLegacyUiEnabled(!settings->legacyUI);
		settings->saveOptions();
		if (settings->guiScale != previousScale && mc != nullptr)
		{
			ScaledResolution sr(settings, mc->displayWidth, mc->displayHeight);
			setWorldAndResolution(mc, sr.getScaledWidth(), sr.getScaledHeight());
			return;
		}
		button->displayString = legacyUiOptionLabel(settings->legacyUI);
		return;
	}
	if (button->id == BUTTON_LEGACY_LOOK)
	{
		settings->legacyLook = !settings->legacyLook;
		button->displayString = uiText("Legacy Look: ") + std::string(settings->legacyLook ? uiText("ON") : uiText("OFF"));
		settings->saveOptions();
		if (mc != nullptr && mc->entityRenderer != nullptr)
			mc->entityRenderer->updateWorldLightLevels();
		return;
	}
	if (button->id == BUTTON_LEGACY_CRAFTING)
	{
		settings->legacyCrafting = !settings->legacyCrafting;
		settings->applyLegacyCraftingBindings();
		button->displayString = uiText("Legacy Crafting: ") + std::string(settings->legacyCrafting ? uiText("ON") : uiText("OFF"));
		settings->saveOptions();
		return;
	}
	if (button->id == BUTTON_LEGACY_CREATIVE)
	{
		settings->legacyCreative = !settings->legacyCreative;
		button->displayString = uiText("Legacy Creative: ") + std::string(settings->legacyCreative ? uiText("ON") : uiText("OFF"));
		settings->saveOptions();
		return;
	}
#ifdef WII_PLATFORM
	if (button->id == BUTTON_ALTERNATIVE_CONTROLS)
	{
		settings->alternativeControllerLayout = !settings->alternativeControllerLayout;
		PlatformUserSettings::setAlternativeControls(settings->alternativeControllerLayout);
		button->displayString = uiText("Alt Controls: ") +
			std::string(settings->alternativeControllerLayout ? uiText("ON") : uiText("OFF"));
		settings->saveOptions();
		return;
	}
#endif
#if PLATFORM_HAS_CONTROLLER_CALIBRATION
	if (button->id == BUTTON_DEADZONE)
	{
		settings->playerName = sanitizeName(nameField != nullptr ? nameField->getText() : "");
		if (mc->session != nullptr)
			mc->session->username = settings->playerName;
		settings->saveOptions();
		mc->displayGuiScreen(new GuiDeadzoneSettings(this, settings));
		return;
	}
#endif
	if (button->id == BUTTON_DONE)
		saveAndClose();
}

void GuiBetaOptions::drawScreen(int_t mouseX, int_t mouseY, float_t partialTick)
{
	drawDefaultBackground();
	int_t topY = height / 6 - 8;
	if (topY < 18) topY = 18;
	drawCenteredString(fontRenderer, uiText("Heritage Options"), width / 2, topY - 12, 0xffffff);
	drawString(fontRenderer, uiText("Player name"), width / 2 - 100, topY + 2, 0xa0a0a0);

	if (nameField != nullptr)
		nameField->drawTextBox();
	GuiScreen::drawScreen(mouseX, mouseY, partialTick);
}
