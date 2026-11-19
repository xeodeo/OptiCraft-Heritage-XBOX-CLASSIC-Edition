#include "net/minecraft/src/UiStrings.h"
#include "GuiControls.h"
#include "GuiSmallButton.h"
#include "GuiButton.h"
#include "GameSettings.h"
#include "KeyBinding.h"
#include "StringTranslate.h"
#include "Minecraft.h"
#include "lwjgl/Keyboard.h"
#include "platform/Input.h"
#include "platform/PlatformConfig.h"
#include "wii/input/GuiWiiControls.h"
#include <algorithm>

GuiControls::GuiControls(GuiScreen *parent, GameSettings *settings)
	: parentScreen(parent)
	, options(settings)
	, screenTitle(uiText("Controls"))
	, buttonId(-1)
	, listTopY(0)
	, listRowStep(24)
	, listButtonHeight(20)
{
}

int_t GuiControls::getLeftEdge() const
{
	return width / 2 - 155;
}

void GuiControls::initGui()
{
	StringTranslate *tr = StringTranslate::getInstance();
	int_t left = getLeftEdge();
	displayedBindings.clear();
	for (int_t j = 0; j < (int_t)options->keyBindings.size(); j++)
	{
		if (options->keyBindings[j] == options->keyBindCrafting && !options->legacyCrafting && !options->xboxStyleCrafting)
			continue;
		displayedBindings.push_back(j);
	}

	const int_t rows = std::max<int_t>(1, ((int_t)displayedBindings.size() + 1) / 2);
	int_t footerRows = 2;
#ifdef WII_PLATFORM
	footerRows = 3;
#endif
	listTopY = height / 6;
	listRowStep = 24;
	listButtonHeight = 20;
	int_t footerStep = 24;
	int_t footerButtonHeight = 20;
	if (listTopY + rows * listRowStep + footerRows * footerStep > height - 4)
	{
		listTopY = 26;
		const int_t footerBudget = footerRows * 20 + 6;
		const int_t avail = std::max<int_t>(rows * 14, height - listTopY - footerBudget);
		listRowStep = std::max<int_t>(14, std::min<int_t>(24, avail / rows));
		listButtonHeight = std::max<int_t>(13, std::min<int_t>(20, listRowStep - 2));
		footerStep = std::max<int_t>(17, std::min<int_t>(24, (height - (listTopY + rows * listRowStep) - 2) / footerRows));
		footerButtonHeight = std::max<int_t>(15, std::min<int_t>(20, footerStep - 2));
	}

	for (int_t k = 0; k < (int_t)displayedBindings.size(); k++)
	{
		int_t j = displayedBindings[k];
		controlList.push_back(new GuiSmallButton(k,
			left + (k % 2) * 160, listTopY + listRowStep * (k >> 1),
			70, listButtonHeight, options->getOptionDisplayString(j)));
	}
	int_t nextY = listTopY + listRowStep * rows;
#ifdef WII_PLATFORM
	controlList.push_back(new GuiButton(201, width / 2 - 100, nextY, 200, footerButtonHeight, uiText("Wii Pad Bindings...")));
	nextY += footerStep;
#endif
	controlList.push_back(new GuiButton(203, width / 2 - 155, nextY, 150, footerButtonHeight, uiText("Reset to Defaults")));
	controlList.push_back(new GuiButton(202, width / 2 + 5, nextY, 150, footerButtonHeight,
		uiText(options->toggleSneak ? "Sneak: Toggle" : "Sneak: Hold")));
	nextY += footerStep;
	controlList.push_back(new GuiButton(200, width / 2 - 100, nextY, 200, footerButtonHeight, tr->translateKey("gui.done")));
	screenTitle = tr->translateKey("controls.title");
}

void GuiControls::onGuiClosed()
{
	platformSetPadRebindExclusive(false);
	buttonId = -1;
}

void GuiControls::actionPerformed(GuiButton *button)
{
	for (int_t k = 0; k < (int_t)displayedBindings.size(); k++)
		controlList[k]->displayString = options->getOptionDisplayString(displayedBindings[k]);

	if (button->id == 203)
	{
		buttonId = -1;
		platformSetPadRebindExclusive(false);
		options->resetControlBindingsToDefaults();
		for (int_t k = 0; k < (int_t)displayedBindings.size(); k++)
			controlList[k]->displayString = options->getOptionDisplayString(displayedBindings[k]);
		return;
	}
	if (button->id == 202)
	{
		options->toggleSneak = !options->toggleSneak;
		options->saveOptions();
		button->displayString = uiText(options->toggleSneak ? "Sneak: Toggle" : "Sneak: Hold");
		return;
	}
	if (button->id == 200)
	{
		// Guard against uiText("Done") being clicked while a binding is still
		// listening (buttonId >= 0) -- otherwise this screen goes away with
		// pad-rebind-exclusive left stuck on, permanently breaking every
		// other menu's button synthesis on PS2.
		platformSetPadRebindExclusive(false);
		mc->displayGuiScreen(parentScreen);
		return;
	}
#ifdef WII_PLATFORM
	if (button->id == 201)
	{
		options->saveOptions();
		mc->displayGuiScreen(new GuiWiiControls(this, options));
		return;
	}
#endif
	if (button->id >= 0 && button->id < (int_t)displayedBindings.size())
	{
		buttonId = button->id;
		int_t bindingIdx = displayedBindings[button->id];
		button->displayString = "> " + options->getOptionDisplayString(bindingIdx) + " <";
		platformSetPadRebindExclusive(true);
	}
}

void GuiControls::keyTyped(char_t c, int_t key)
{
	if (buttonId >= 0 && buttonId < (int_t)displayedBindings.size())
	{
		int_t bindingIdx = displayedBindings[buttonId];
#if PLATFORM_XBOX || PLATFORM_PS2
		if (key == lwjgl::Keyboard::KEY_ESCAPE)
		{
			controlList[buttonId]->displayString = options->getOptionDisplayString(bindingIdx);
			buttonId = -1;
			platformSetPadRebindExclusive(false);
			return;
		}
#endif
		options->setKeyBinding(bindingIdx, key);
		controlList[buttonId]->displayString = options->getOptionDisplayString(bindingIdx);
		buttonId = -1;
		platformSetPadRebindExclusive(false);
	}
	else
	{
		GuiScreen::keyTyped(c, key);
	}
}


void GuiControls::mouseClicked(int_t x, int_t y, int_t button)
{
	if (buttonId >= 0 && buttonId < (int_t)displayedBindings.size())
	{
#if PLATFORM_XBOX || PLATFORM_PS2 || PLATFORM_WII
		(void)x;
		(void)y;
		(void)button;
		return;
#else
		int_t bindingIdx = displayedBindings[buttonId];
		options->setKeyBinding(bindingIdx, -100 + button);
		controlList[buttonId]->displayString = options->getOptionDisplayString(bindingIdx);
		buttonId = -1;
		platformSetPadRebindExclusive(false);
		return;
#endif
	}
	GuiScreen::mouseClicked(x, y, button);
}

void GuiControls::drawScreen(int_t mouseX, int_t mouseY, float_t partialTick)
{
	drawDefaultBackground();
	drawCenteredString(fontRenderer, screenTitle, width / 2, listTopY <= 28 ? 10 : 20, 0xffffff);
	int_t left = getLeftEdge();
	const int_t labelOffsetY = std::max<int_t>(2, (listButtonHeight - 8) / 2);
	for (int_t k = 0; k < (int_t)displayedBindings.size(); k++)
	{
		int_t l = displayedBindings[k];
		bool duplicate = false;
		if (options->keyBindings[l]->keyCode != lwjgl::Keyboard::KEY_NONE)
		{
			for (int_t otherK = 0; otherK < (int_t)displayedBindings.size(); ++otherK)
			{
				int_t other = displayedBindings[otherK];
				if (other != l && options->keyBindings[l]->keyCode == options->keyBindings[other]->keyCode)
				{
					duplicate = true;
					break;
				}
			}
		}

		if (buttonId == k)
			controlList[k]->displayString = "\xc2\xa7" "f> " "\xc2\xa7" "e??? " "\xc2\xa7" "f<";
		else if (duplicate)
			controlList[k]->displayString = std::string("\xc2\xa7" "c") + options->getOptionDisplayString(l);
		else
			controlList[k]->displayString = options->getOptionDisplayString(l);

		drawString(fontRenderer, options->getKeyBindingDescription(l),
		           left + (k % 2) * 160 + 70 + 6, listTopY + listRowStep * (k >> 1) + labelOffsetY, -1);
	}
	GuiScreen::drawScreen(mouseX, mouseY, partialTick);
}

bool GuiControls::isJavaUiKeyboardNavigationEnabled() const
{
	return buttonId < 0 && GuiScreen::isJavaUiKeyboardNavigationEnabled();
}
