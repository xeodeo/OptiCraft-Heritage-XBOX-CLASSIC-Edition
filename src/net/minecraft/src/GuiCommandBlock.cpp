#include "GuiCommandBlock.h"

#include <algorithm>
#include <string>

#include "FontRenderer.h"
#include "GameSettings.h"
#include "GuiButton.h"
#include "GuiTextField.h"
#include "Minecraft.h"
#include "StatCollector.h"
#include "TileEntityCommandBlock.h"
#include "pc/lwjgl/Keyboard.h"

namespace
{
std::string trOrDefault(const char *key, const char *fallback)
{
	std::string s = StatCollector::translateToLocal(key);
	if (s.empty() || s == key)
		return fallback;
	return s;
}
} // namespace

GuiCommandBlock::GuiCommandBlock(TileEntityCommandBlock *tile)
	: commandTextField(nullptr)
	, commandBlock(tile)
	, doneBtn(nullptr)
	, cancelBtn(nullptr)
{
}

GuiCommandBlock::~GuiCommandBlock()
{
	if (commandTextField != nullptr)
		commandTextField->setFocused(false);
	delete commandTextField;
	commandTextField = nullptr;
}

void GuiCommandBlock::initGui()
{
	lwjgl::Keyboard::enableRepeatEvents(true);
	controlList.clear();

	const bool legacy = (mc != nullptr && mc->gameSettings != nullptr && mc->gameSettings->legacyUI);
	const bool compact = (height < 170);
	const int_t fieldW = std::min<int_t>(300, width - 36);
	const int_t fieldX = (width - fieldW) / 2;
	const int_t fieldY = compact ? 30 : (legacy ? (height / 2 - 22) : 60);
	const int_t btnW = std::min<int_t>(150, (width - 40) / 2);
	const int_t btnY = compact ? std::max<int_t>(fieldY + 26, height - 26)
	                           : (legacy ? (height / 2 + 48) : (height / 4 + 96 + 12));

	doneBtn = new GuiButton(0, width / 2 - btnW - 4, btnY, btnW, 20, trOrDefault("gui.done", "Done"));
	cancelBtn = new GuiButton(1, width / 2 + 4, btnY, btnW, 20, trOrDefault("gui.cancel", "Cancel"));
	controlList.push_back(doneBtn);
	controlList.push_back(cancelBtn);

	if (commandTextField != nullptr)
		commandTextField->setFocused(false);
	delete commandTextField;
	commandTextField = new GuiTextField(this, fontRenderer, fieldX, fieldY, fieldW, 20,
	                                    commandBlock != nullptr ? commandBlock->getCommand() : "");
	commandTextField->setMaxStringLength(256);
	commandTextField->setFocused(true);
}

void GuiCommandBlock::onGuiClosed()
{
	lwjgl::Keyboard::enableRepeatEvents(false);
	if (commandTextField != nullptr)
		commandTextField->setFocused(false);
}

void GuiCommandBlock::updateScreen()
{
	if (commandTextField != nullptr)
		commandTextField->updateCursorCounter();
}

void GuiCommandBlock::actionPerformed(GuiButton *button)
{
	if (button == nullptr || !button->enabled)
		return;

	if (button->id == 1)
	{
		mc->displayGuiScreen(nullptr);
	}
	else if (button->id == 0)
	{
		if (commandBlock != nullptr && commandTextField != nullptr)
		{
			commandBlock->setCommand(commandTextField->getText());
		}
		mc->displayGuiScreen(nullptr);
	}
}

void GuiCommandBlock::keyTyped(char_t c, int_t key)
{
	if (commandTextField != nullptr && commandTextField->textboxKeyTyped(c, key))
	{
		return;
	}

	if (key == 28 || key == 156) // Enter / Numpad Enter
	{
		actionPerformed(doneBtn);
	}
	else if (key == 1) // Escape
	{
		actionPerformed(cancelBtn);
	}
}

void GuiCommandBlock::mouseClicked(int_t mouseX, int_t mouseY, int_t button)
{
	GuiScreen::mouseClicked(mouseX, mouseY, button);
	if (commandTextField != nullptr)
	{
		commandTextField->mouseClicked(mouseX, mouseY, button);
	}
}

void GuiCommandBlock::drawScreen(int_t mouseX, int_t mouseY, float_t partialTick)
{
	const bool legacy = (mc != nullptr && mc->gameSettings != nullptr && mc->gameSettings->legacyUI);
	const bool compact = (height < 170);
	drawDefaultBackground();

	const std::string title = trOrDefault("advMode.setCommand", "Set Console Command for Block");
	const std::string cmdLabel = trOrDefault("advMode.command", "Console Command");
	const std::string nearestHint = trOrDefault("advMode.nearestPlayer", "Use \"@p\" to target nearest player");
	const std::string randomHint = trOrDefault("advMode.randomPlayer", "Use \"@r\" to target random player");
	const std::string allHint = trOrDefault("advMode.allPlayers", "Use \"@a\" to target all players");

	if (legacy)
	{
		const int_t panelW = std::min<int_t>(332, width - 16);
		const int_t panelH = compact ? std::min<int_t>(152, height - 8) : 152;
		const int_t x0 = (width - panelW) / 2;
		const int_t y0 = compact ? (height - panelH) / 2 : (height / 2 - 74);
		const int_t x1 = x0 + panelW;
		const int_t y1 = y0 + panelH;

		// Classic Legacy Console 3D raised panel
		drawRect(x0 - 2, y0 - 2, x1 + 2, y1 + 2, 0xFF000000);
		drawRect(x0, y0, x1, y1, 0xFFC6C6C6);
		drawRect(x0, y0, x1 - 2, y0 + 2, 0xFFFFFFFF);
		drawRect(x0, y0, x0 + 2, y1 - 2, 0xFFFFFFFF);
		drawRect(x0 + 2, y1 - 2, x1, y1, 0xFF555555);
		drawRect(x1 - 2, y0 + 2, x1, y1, 0xFF555555);

		const int_t titleW = fontRenderer->getStringWidth(title);
		fontRenderer->drawString(title, (width - titleW) / 2, compact ? (y0 + 5) : (y0 + 10), 0x404040);
		fontRenderer->drawString(cmdLabel, x0 + 16, compact ? 19 : (height / 2 - 35), 0x404040);

		if (commandTextField != nullptr)
			commandTextField->drawTextBox();

		if (!compact)
		{
			int_t hintY = height / 2 + 4;
			fontRenderer->drawString(nearestHint, x0 + 16, hintY, 0x555555);
			hintY += 11;
			fontRenderer->drawString(randomHint, x0 + 16, hintY, 0x555555);
			hintY += 11;
			fontRenderer->drawString(allHint, x0 + 16, hintY, 0x555555);
		}
		else if (height >= 105)
		{
			fontRenderer->drawString("@p / @r / @a", x0 + 16, 54, 0x555555);
		}
	}
	else
	{
		drawCenteredString(fontRenderer, title, width / 2, compact ? 6 : 20, 0xFFFFFF);
		const int_t leftX = (width - std::min<int_t>(300, width - 36)) / 2;
		drawString(fontRenderer, cmdLabel, leftX, compact ? 18 : 47, 0xA0A0A0);

		if (commandTextField != nullptr)
			commandTextField->drawTextBox();

		if (!compact)
		{
			int_t hintY = 92;
			drawString(fontRenderer, nearestHint, leftX, hintY, 0xA0A0A0);
			hintY += 11;
			drawString(fontRenderer, randomHint, leftX, hintY, 0xA0A0A0);
			hintY += 11;
			drawString(fontRenderer, allHint, leftX, hintY, 0xA0A0A0);
		}
		else if (height >= 105)
		{
			drawString(fontRenderer, "@p / @r / @a", leftX, 54, 0xA0A0A0);
		}
	}

	GuiScreen::drawScreen(mouseX, mouseY, partialTick);
}
