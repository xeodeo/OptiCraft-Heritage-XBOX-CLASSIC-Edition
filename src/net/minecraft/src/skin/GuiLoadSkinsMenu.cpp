#include "GuiLoadSkinsMenu.h"
#include "GuiLoadSkinsList.h"
#include "GuiSearchSkin.h"
#include "GuiButton.h"
#include "Minecraft.h"
#include "FontRenderer.h"
#include "SoundManager.h"
#include "StringTranslate.h"

#if PLATFORM_PS2 || PLATFORM_WII || PLATFORM_XBOX
#include "platform/Input.h"
#endif

GuiLoadSkinsMenu::GuiLoadSkinsMenu(GuiScreen *parent)
    : parentScreen(parent)
    , selectedButtonIndex(0)
{
}

void GuiLoadSkinsMenu::initGui()
{
    controlList.clear();

    StringTranslate *tr = StringTranslate::getInstance();
    const bool es = (tr != nullptr && tr->getCurrentLanguage().rfind("es_", 0) == 0);

    const int_t btnW = std::min<int_t>(220, width - 24);
    const int_t btnH = 20;
    const int_t centerX = width / 2 - btnW / 2;
    const int_t startY = height / 4 + 22;

    controlList.push_back(new GuiButton(4, centerX, startY, btnW, btnH, es ? "Buscar skin por usuario (Online)" : "Search Skin by Username (Online)"));
    controlList.push_back(new GuiButton(2, centerX, startY + 26, btnW, btnH, es ? "Cargar desde consola / FTP" : "Load from Device (Recommended)"));
    controlList.push_back(new GuiButton(1, centerX, startY + 52, btnW, btnH, es ? "Cargar desde USB" : "Load from USB Storage"));
    controlList.push_back(new GuiButton(3, centerX, startY + 84, btnW, btnH, es ? "Volver" : "Back"));
}

void GuiLoadSkinsMenu::actionPerformed(GuiButton *button)
{
    if (!button->enabled)
        return;

    if (mc != nullptr && mc->sndManager != nullptr)
        mc->sndManager->playSoundFX("random.click", 1.0f, 1.0f);

    if (button->id == 4) // Online search
    {
        const bool p2 = (mc != nullptr && mc->isScreenOwnedByPlayer2());
        mc->displayGuiScreen(new GuiSearchSkin(parentScreen != nullptr ? parentScreen : this, p2));
    }
    else if (button->id == 1) // USB
    {
        mc->displayGuiScreen(new GuiLoadSkinsList(this, GuiLoadSkinsList::Source::USB));
    }
    else if (button->id == 2) // Device
    {
        mc->displayGuiScreen(new GuiLoadSkinsList(this, GuiLoadSkinsList::Source::Device));
    }
    else if (button->id == 3) // Back
    {
        mc->displayGuiScreen(parentScreen);
    }
}

void GuiLoadSkinsMenu::keyTyped(char_t c, int_t key)
{
    if (key == 1) // ESC
    {
        mc->displayGuiScreen(parentScreen);
        return;
    }
    GuiScreen::keyTyped(c, key);
}

void GuiLoadSkinsMenu::handleSpecializedMenuInput()
{
#if PLATFORM_PS2 || PLATFORM_WII || PLATFORM_XBOX
    const PlatformTextInputSnapshot pad = platformTextInputSnapshot(platformMenuPad());
    if (!pad.connected)
        return;

    if ((pad.pressed & PLATFORM_TEXT_UP) != 0)
    {
        if (selectedButtonIndex > 0)
            selectedButtonIndex--;
        else
            selectedButtonIndex = static_cast<int>(controlList.size()) - 1;

        if (mc != nullptr && mc->sndManager != nullptr)
            mc->sndManager->playSoundFX("random.click", 1.0f, 1.2f);
    }
    else if ((pad.pressed & PLATFORM_TEXT_DOWN) != 0)
    {
        if (selectedButtonIndex < static_cast<int>(controlList.size()) - 1)
            selectedButtonIndex++;
        else
            selectedButtonIndex = 0;

        if (mc != nullptr && mc->sndManager != nullptr)
            mc->sndManager->playSoundFX("random.click", 1.0f, 1.2f);
    }

    if ((pad.pressed & (PLATFORM_TEXT_TYPE | PLATFORM_TEXT_ENTER)) != 0)
    {
        if (selectedButtonIndex >= 0 && selectedButtonIndex < static_cast<int>(controlList.size()))
            actionPerformed(controlList[selectedButtonIndex]);
    }
    else if ((pad.pressed & (PLATFORM_TEXT_BACK | PLATFORM_TEXT_CLOSE)) != 0)
    {
        if (mc != nullptr && mc->sndManager != nullptr)
            mc->sndManager->playSoundFX("random.back", 1.0f, 1.0f);
        mc->displayGuiScreen(parentScreen);
    }
#endif
}

void GuiLoadSkinsMenu::drawScreen(int_t mouseX, int_t mouseY, float_t partialTick)
{
    drawDefaultBackground();

    StringTranslate *tr = StringTranslate::getInstance();
    const bool es = (tr != nullptr && tr->getCurrentLanguage().rfind("es_", 0) == 0);

    drawCenteredString(fontRenderer, es ? "Cargar Skins" : "Load Skins", width / 2, height / 4 - 4, 0xFFFFFF);
    drawCenteredString(fontRenderer, es ? "Descarga por usuario o copia .png en la carpeta 'skins'"
                                        : "Search online by username or place .png skins in 'skins'",
                       width / 2, height / 4 + 10, 0xA0A0A0);

    GuiScreen::drawScreen(mouseX, mouseY, partialTick);
}
