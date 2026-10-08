#include "GuiTexturePacks.h"
#include "GuiTexturePackSlot.h"
#include "GuiSmallButton.h"
#include "GuiButton.h"
#include "StringTranslate.h"
#include "TexturePackList.h"
#include "RenderEngine.h"
#include "FontRenderer.h"
#include "Minecraft.h"
#include "java/File.h"
#include "java/System.h"
#include "java/String.h"
#include "SoundManager.h"
#include "platform/PlatformConfig.h"
#if PLATFORM_PS2 || PLATFORM_WII || PLATFORM_XBOX
#include "platform/Input.h"
#endif
#include <memory>

GuiTexturePacks::GuiTexturePacks(GuiScreen *guiscreen)
	: field_6454_o(-1)
	, controllerIndex(0)
	, fileLocation("")
	, guiScreen(guiscreen)
	, guiTexturePackSlot(nullptr)
{
}

GuiTexturePacks::~GuiTexturePacks()
{
	delete guiTexturePackSlot;
}

void GuiTexturePacks::initGui()
{
	StringTranslate *stringtranslate = StringTranslate::getInstance();
	controlList.push_back(new GuiSmallButton(5, width / 2 - 154, height - 48, stringtranslate->translateKey("texturePack.openFolder")));
	controlList.push_back(new GuiSmallButton(6, width / 2 + 4,   height - 48, stringtranslate->translateKey("gui.done")));
	mc->texturePackList->updateAvailableTexturePacks();
	std::unique_ptr<File> texturePackDirectory(File::open(*Minecraft::getMinecraftDir(), "texturepacks"));
	fileLocation = texturePackDirectory->toString();
	delete guiTexturePackSlot;
	guiTexturePackSlot = new GuiTexturePackSlot(this);
	guiTexturePackSlot->registerScrollButtons(controlList, 7, 8);

	// The pad starts on the pack in use.
	controllerIndex = 0;
	const auto &packs = mc->texturePackList->getAvailableTexturePacks();
	for (std::size_t i = 0; i < packs.size(); ++i)
		if (packs[i] == mc->texturePackList->getSelectedTexturePack())
			controllerIndex = static_cast<int_t>(i);
}

void GuiTexturePacks::handleSpecializedMenuInput()
{
#if PLATFORM_PS2 || PLATFORM_WII || PLATFORM_XBOX
	// Like the server list: up/down walk the packs, A applies the highlighted
	// one, B is Done.
	const PlatformTextInputSnapshot pad = platformTextInputSnapshot(platformMenuPad());
	const int_t count = static_cast<int_t>(mc->texturePackList->getAvailableTexturePacks().size());
	if ((pad.pressed & (PLATFORM_TEXT_CLOSE | PLATFORM_TEXT_SHIFT)) != 0)
	{
		mc->sndManager->playSoundFX("random.back", 1.0f, 1.0f);
		mc->refreshResources();
		mc->displayGuiScreen(guiScreen);
		return;
	}
	int_t step = 0;
	if ((pad.pressed & PLATFORM_TEXT_UP) != 0) step = -1;
	else if ((pad.pressed & PLATFORM_TEXT_DOWN) != 0) step = 1;
	if (step != 0 && count > 0)
	{
		const int_t next = controllerIndex + step;
		if (next >= 0 && next < count)
		{
			controllerIndex = next;
			guiTexturePackSlot->scrollToElement(controllerIndex);
			mc->sndManager->playSoundFX("random.focus", 1.0f, 1.0f);
		}
	}
	if ((pad.pressed & (PLATFORM_TEXT_TYPE | PLATFORM_TEXT_ENTER)) != 0 &&
		controllerIndex >= 0 && controllerIndex < count)
	{
		mc->sndManager->playSoundFX("random.action", 1.0f, 1.0f);
		const auto &packs = mc->texturePackList->getAvailableTexturePacks();
		mc->texturePackList->setTexturePack(packs[static_cast<std::size_t>(controllerIndex)]);
		mc->refreshResources();
	}
#endif
}

void GuiTexturePacks::actionPerformed(GuiButton *guibutton)
{
	if (!guibutton->enabled) return;
	if (guibutton->id == 5)
	{
		System::openURL("file://" + fileLocation);
	}
	else if (guibutton->id == 6)
	{
		mc->refreshResources();
		mc->displayGuiScreen(guiScreen);
	}
	else
	{
		guiTexturePackSlot->actionPerformed(guibutton);
	}
}

void GuiTexturePacks::mouseClicked(int_t i, int_t j, int_t k)
{
	GuiScreen::mouseClicked(i, j, k);
}

void GuiTexturePacks::mouseMovedOrUp(int_t i, int_t j, int_t k)
{
	GuiScreen::mouseMovedOrUp(i, j, k);
}

void GuiTexturePacks::drawScreen(int_t i, int_t j, float_t f)
{
	guiTexturePackSlot->drawScreen(i, j, f);
	if (field_6454_o <= 0)
	{
		mc->texturePackList->updateAvailableTexturePacks();
		field_6454_o += 20;
	}
	StringTranslate *stringtranslate = StringTranslate::getInstance();
	drawCenteredString(fontRenderer, stringtranslate->translateKey("texturePack.title"),      width / 2,      16,            0xffffff);
	drawCenteredString(fontRenderer, stringtranslate->translateKey("texturePack.folderInfo"), width / 2 - 77, height - 26,   0x808080);
	GuiScreen::drawScreen(i, j, f);
}

void GuiTexturePacks::updateScreen()
{
	GuiScreen::updateScreen();
	field_6454_o--;
}
