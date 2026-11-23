#include "GuiTexturePacks.h"
#include "GuiTexturePackSlot.h"
#include "GuiSmallButton.h"
#include "GuiButton.h"
#include "StringTranslate.h"
#include "TexturePackList.h"
#include "TexturePackBase.h"
#include "RenderEngine.h"
#include "FontRenderer.h"
#include "Minecraft.h"
#include "GameSettings.h"
#include "Tessellator.h"
#include "ControlIcon.h"
#include "java/File.h"
#include "java/System.h"
#include "java/String.h"
#include "SoundManager.h"
#include "legacy/LegacyMenuWidgets.h"
#include "legacy/LegacyOptionText.h"
#include "texdl/GuiTexturePackDownload.h"
#include "platform/PlatformConfig.h"
#include "platform/RenderAPI.h"
#if PLATFORM_PS2 || PLATFORM_WII || PLATFORM_XBOX
#include "platform/Input.h"
#endif
#include <algorithm>
#include <memory>

namespace
{
bool packsSpanish()
{
	StringTranslate *tr = StringTranslate::getInstance();
	return tr != nullptr && tr->getCurrentLanguage().rfind("es_", 0) == 0;
}

const char *downloadLabel()
{
	return packsSpanish() ? "Descargar de internet" : "Download from the internet";
}
}

GuiTexturePacks::GuiTexturePacks(GuiScreen *guiscreen)
	: guiScreen(guiscreen)
	, field_6454_o(-1)
	, controllerIndex(0)
	, legacyScroll(0)
	, inputLatch(true)
	, fileLocation("")
	, guiTexturePackSlot(nullptr)
{
}

GuiTexturePacks::~GuiTexturePacks()
{
	delete guiTexturePackSlot;
}

bool GuiTexturePacks::isLegacyMode() const
{
	return mc != nullptr && mc->gameSettings != nullptr && mc->gameSettings->legacyUI;
}

bool GuiTexturePacks::usesSpecializedMenuNavigation() const
{
	return isLegacyMode();
}

void GuiTexturePacks::initGui()
{
	StringTranslate *stringtranslate = StringTranslate::getInstance();
	inputLatch = true;
	if (!isLegacyMode())
	{
#if PLATFORM_XBOX
		// No folder to open on the console: the left button downloads packs instead.
		controlList.push_back(new GuiSmallButton(5, width / 2 - 154, height - 48, downloadLabel()));
#else
		controlList.push_back(new GuiSmallButton(5, width / 2 - 154, height - 48, stringtranslate->translateKey("texturePack.openFolder")));
#endif
		controlList.push_back(new GuiSmallButton(6, width / 2 + 4,   height - 48, stringtranslate->translateKey("gui.done")));
	}
	mc->texturePackList->updateAvailableTexturePacks();
	std::unique_ptr<File> texturePackDirectory(File::open(*Minecraft::getMinecraftDir(), "texturepacks"));
	fileLocation = texturePackDirectory->toString();
	delete guiTexturePackSlot;
	guiTexturePackSlot = new GuiTexturePackSlot(this);
	if (!isLegacyMode())
		guiTexturePackSlot->registerScrollButtons(controlList, 7, 8);

	// The pad starts on the pack in use.
	controllerIndex = 0;
	const auto &packs = mc->texturePackList->getAvailableTexturePacks();
	for (std::size_t i = 0; i < packs.size(); ++i)
		if (packs[i] == mc->texturePackList->getSelectedTexturePack())
			controllerIndex = static_cast<int_t>(i);
}

void GuiTexturePacks::openDownloader()
{
#if PLATFORM_XBOX
	mc->sndManager->playSoundFX("random.click", 1.0f, 1.0f);
	mc->displayGuiScreen(new GuiTexturePackDownload(this));
#endif
}

void GuiTexturePacks::applyHighlighted()
{
	const auto &packs = mc->texturePackList->getAvailableTexturePacks();
	if (controllerIndex < 0 || controllerIndex >= static_cast<int_t>(packs.size()))
		return;
	mc->sndManager->playSoundFX("random.action", 1.0f, 1.0f);
	mc->texturePackList->setTexturePack(packs[static_cast<std::size_t>(controllerIndex)]);
	mc->refreshResources();
}

void GuiTexturePacks::handleSpecializedMenuInput()
{
#if PLATFORM_PS2 || PLATFORM_WII || PLATFORM_XBOX
	// Like the server list: up/down walk the packs, A applies the highlighted
	// one, B is Done. X (Xbox) opens the downloader; in the Legacy list the
	// row after the last pack is the download button.
	const PlatformTextInputSnapshot pad = platformTextInputSnapshot(platformMenuPad());
	if (!pad.connected)
		return;
	if (inputLatch)
	{
		if ((pad.held & (PLATFORM_TEXT_TYPE | PLATFORM_TEXT_CLOSE | PLATFORM_TEXT_BACK | PLATFORM_TEXT_SHIFT)) == 0)
			inputLatch = false;
		return;
	}
	const bool legacy = isLegacyMode();
	const int_t count = static_cast<int_t>(mc->texturePackList->getAvailableTexturePacks().size());
	const int_t last = legacy && PLATFORM_XBOX ? count : count - 1;
	if ((pad.pressed & (PLATFORM_TEXT_CLOSE | PLATFORM_TEXT_SHIFT)) != 0)
	{
		mc->sndManager->playSoundFX("random.back", 1.0f, 1.0f);
		mc->refreshResources();
		mc->displayGuiScreen(guiScreen);
		return;
	}
#if PLATFORM_XBOX
	if ((pad.pressed & PLATFORM_TEXT_BACK) != 0)
	{
		openDownloader();
		return;
	}
#endif
	int_t step = 0;
	if ((pad.pressed & PLATFORM_TEXT_UP) != 0) step = -1;
	else if ((pad.pressed & PLATFORM_TEXT_DOWN) != 0) step = 1;
	if (step != 0 && last >= 0)
	{
		const int_t next = controllerIndex + step;
		if (next >= 0 && next <= last)
		{
			controllerIndex = next;
			if (!legacy && controllerIndex < count)
				guiTexturePackSlot->scrollToElement(controllerIndex);
			mc->sndManager->playSoundFX("random.focus", 1.0f, 1.0f);
		}
	}
	if ((pad.pressed & (PLATFORM_TEXT_TYPE | PLATFORM_TEXT_ENTER)) != 0)
	{
		if (legacy && controllerIndex == count)
			openDownloader();
		else
			applyHighlighted();
	}
#endif
}

void GuiTexturePacks::actionPerformed(GuiButton *guibutton)
{
	if (!guibutton->enabled) return;
	if (guibutton->id == 5)
	{
#if PLATFORM_XBOX
		openDownloader();
#else
		System::openURL("file://" + fileLocation);
#endif
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

void GuiTexturePacks::drawLegacyScreen(float_t partialTick)
{
	const bool es = packsSpanish();
	const auto &packs = mc->texturePackList->getAvailableTexturePacks();
	const int_t count = static_cast<int_t>(packs.size());
	const bool wide = width >= 420;
	const int_t rowH = height >= 330 ? 26 : 24;
	const int_t headerH = 20;
	const int_t buttonH = height >= 330 ? 20 : 18;
	const int_t footerH = PLATFORM_XBOX ? buttonH + 8 : 4;
	const LegacyMenuLayout l = legacyMenuLayout(mc, width, height, wide ? 420 : 300, std::max<int_t>(count, 3),
		headerH, footerH, rowH);
	legacyDrawMenuBackdrop(mc, width, height, partialTick);
	legacyDrawMenuPanel(l.panelX, l.panelY, l.panelW, l.panelH);
	legacyDrawPanelTitle(fontRenderer, StringTranslate::getInstance()->translateKey("texturePack.title"),
		l.panelX + l.panelW / 2, l.titleY);

	const int_t listW = l.contentW;
	const int_t rowW = listW - 7;
	const int_t listH = l.visibleRows * l.rowH + (l.visibleRows - 1) * l.rowGap;
	if (controllerIndex < count)
		legacyScroll = legacyMenuKeepVisible(controllerIndex, count, l.visibleRows, legacyScroll);
	const int_t rows = std::min<int_t>(l.visibleRows, count - legacyScroll);
	TexturePackBase *inUse = mc->texturePackList->getSelectedTexturePack();
	for (int_t r = 0; r < rows; ++r)
	{
		const int_t idx = legacyScroll + r;
		TexturePackBase *pack = packs[static_cast<std::size_t>(idx)];
		const int_t ry = l.rowY(r);
		const bool sel = idx == controllerIndex;
		const int_t labelColor = legacyDrawMenuButtonFrame(mc, l.contentX, ry, rowW, l.rowH, sel, pack->isUsable());

		// pack.png thumbnail.
		const int_t thumb = l.rowH - 4;
		const int_t tx = l.contentX + 3;
		const int_t ty = ry + 2;
		pack->bindThumbnailTexture(mc);
		renderColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		Tessellator *tess = &Tessellator::instance;
		tess->startDrawingQuads();
		tess->setColorOpaque_I(0xffffff);
		tess->addVertexWithUV(tx,         ty + thumb, 0.0, 0.0, 1.0);
		tess->addVertexWithUV(tx + thumb, ty + thumb, 0.0, 1.0, 1.0);
		tess->addVertexWithUV(tx + thumb, ty,         0.0, 1.0, 0.0);
		tess->addVertexWithUV(tx,         ty,         0.0, 0.0, 0.0);
		tess->draw();

		// The one in use: ticked box on the right, like the mods catalog.
		const int_t tickX = l.contentX + rowW - 17;
		if (pack == inUse)
			legacyDrawTickBox(mc, tickX, ry + (l.rowH - 12) / 2, true, sel);
		const int_t textX = tx + thumb + 5;
		const int_t textW = tickX - 4 - textX;
		const int_t top = ry + (l.rowH - 18) / 2;
		legacyDrawOptionText(fontRenderer, legacyFitText(fontRenderer, pack->texturePackFileName, textW), textX, top, labelColor);
		std::string line = pack->firstDescriptionLine;
		if (!pack->secondDescriptionLine.empty())
			line += line.empty() ? pack->secondDescriptionLine : " " + pack->secondDescriptionLine;
		legacyDrawOptionText(fontRenderer, legacyFitText(fontRenderer, line, textW), textX, top + 10,
			sel ? static_cast<int_t>(0xffe8e8e8u) : static_cast<int_t>(0xffc0c0c0u));
	}
	legacyDrawMenuScrollbar(l.contentX + listW - 5, l.firstRowY, listH, legacyScroll, l.visibleRows, count);

#if PLATFORM_XBOX
	// Download button under the list (reached with D-pad down, or X anywhere).
	const int_t bw = std::min<int_t>(200, l.contentW);
	legacyDrawMenuButton(mc, l.panelX + (l.panelW - bw) / 2, l.footerY + 2, bw, buttonH, downloadLabel(),
		controllerIndex == count, true);
	const std::string btns[] = {"D-Pad", "A", "X", "B"};
	const std::string acts[] = {
		es ? "Mover" : "Move",
		controllerIndex == count ? (es ? "Abrir" : "Open") : (es ? "Usar" : "Use"),
		es ? "Descargar" : "Download",
		es ? "Volver" : "Back"
	};
	drawControlHintRow(mc, width, l.hintY, btns, acts, 4);
#else
	(void)es;
#endif
}

void GuiTexturePacks::drawScreen(int_t i, int_t j, float_t f)
{
	if (field_6454_o <= 0)
	{
		mc->texturePackList->updateAvailableTexturePacks();
		field_6454_o += 20;
	}
	if (isLegacyMode())
	{
		drawLegacyScreen(f);
		return;
	}
	guiTexturePackSlot->drawScreen(i, j, f);
	StringTranslate *stringtranslate = StringTranslate::getInstance();
	drawCenteredString(fontRenderer, stringtranslate->translateKey("texturePack.title"),      width / 2,      16,            0xffffff);
#if PLATFORM_XBOX
	drawCenteredString(fontRenderer, packsSpanish() ? "X: descargar packs de internet" : "X: download packs from the internet",
		width / 2 - 77, height - 26, 0x808080);
#else
	drawCenteredString(fontRenderer, stringtranslate->translateKey("texturePack.folderInfo"), width / 2 - 77, height - 26,   0x808080);
#endif
	GuiScreen::drawScreen(i, j, f);
}

void GuiTexturePacks::updateScreen()
{
	GuiScreen::updateScreen();
	field_6454_o--;
}
