#include "GuiChocoboMenu.h"
#include "EntityChocobo.h"

#include "net/minecraft/src/Minecraft.h"
#include "net/minecraft/src/GameSettings.h"
#include "net/minecraft/src/EntityPlayer.h"
#include "net/minecraft/src/InventoryPlayer.h"
#include "net/minecraft/src/GuiButton.h"
#include "net/minecraft/src/GuiChest.h"
#include "net/minecraft/src/GuiTextField.h"
#include "net/minecraft/src/FontRenderer.h"
#include "net/minecraft/src/SoundManager.h"
#include "net/minecraft/src/StringTranslate.h"
#include "net/minecraft/src/ControlIcon.h"
#include "net/minecraft/src/legacy/LegacyMenuHints.h"
#include "net/minecraft/src/legacy/LegacyOptionsLayout.h"
#include "net/minecraft/src/legacy/LegacyOptionsPanel.h"
#include "platform/PlatformConfig.h"
#include "platform/Input.h"

#if defined(PS2_PLATFORM) || defined(WII_PLATFORM) || defined(XBOX_PLATFORM)
#include "net/minecraft/src/VirtualKeyboard.h"
#endif

#if !PLATFORM_PS2 && !PLATFORM_WII && !PLATFORM_XBOX
#include "pc/lwjgl/Keyboard.h"
#endif

#include <algorithm>
#include <cstdio>

namespace
{
bool chocoMenuIsSpanish()
{
    StringTranslate *tr = StringTranslate::getInstance();
    return (tr != nullptr && tr->getCurrentLanguage().rfind("es_", 0) == 0);
}
}

GuiChocoboMenu::GuiChocoboMenu(EntityChocobo *chocobo, EntityPlayer *player, int playerIndex)
    : m_chocobo(chocobo)
    , m_player(player)
    , m_playerIndex(playerIndex)
    , m_selectedIndex(0)
    , m_renaming(false)
    , m_openLatch(true)
    , m_nameField(nullptr)
{
}

GuiChocoboMenu::~GuiChocoboMenu()
{
    delete m_nameField;
    m_nameField = nullptr;
}

bool GuiChocoboMenu::isLegacyMode() const
{
    return mc != nullptr && mc->gameSettings != nullptr && mc->gameSettings->legacyUI;
}

bool GuiChocoboMenu::usesSpecializedMenuNavigation() const
{
    if (m_renaming)
        return false;
#if PLATFORM_PS2 || PLATFORM_WII || PLATFORM_XBOX
    return true;
#else
    return isLegacyMode();
#endif
}

void GuiChocoboMenu::initGui()
{
    m_openLatch = true;
    m_renaming = false;
    controlList.clear();

    delete m_nameField;
    const int_t fw = std::min<int_t>(180, width - 40);
    m_nameField = new GuiTextField(this, fontRenderer, (width - fw) / 2, height / 2 - 44, fw, 18, "");
    m_nameField->setMaxStringLength(20);
    m_nameField->setFocused(false);

    if (!isLegacyMode())
    {
        const bool isSplit = (mc != nullptr && mc->isSplitScreenActive());
        const int_t hintY = legacyHintRowY(height, isSplit);
        const int_t panelH = std::min<int_t>(168, hintY - 14);
        const int_t panelY = std::max<int_t>(10, (hintY - panelH) / 2);
        const int_t bx = width / 2 - 100;
        const int_t startY = panelY + 34;
        controlList.push_back(new GuiButton(0, bx, startY + 0, 200, 20, ""));
        controlList.push_back(new GuiButton(1, bx, startY + 23, 200, 20, ""));
        controlList.push_back(new GuiButton(2, bx, startY + 46, 200, 20, ""));
        controlList.push_back(new GuiButton(3, bx, startY + 69, 200, 20, ""));
        controlList.push_back(new GuiButton(200, bx, panelY + panelH - 24, 200, 20, chocoMenuIsSpanish() ? "Cerrar" : "Done"));
        refreshJavaButtons();
    }
}

void GuiChocoboMenu::onGuiClosed()
{
#if defined(PS2_PLATFORM) || defined(WII_PLATFORM) || defined(XBOX_PLATFORM)
    VirtualKeyboard::instance().releaseFocus();
#endif
    if (m_nameField != nullptr)
        m_nameField->setFocused(false);
    GuiScreen::onGuiClosed();
}

void GuiChocoboMenu::updateScreen()
{
    GuiScreen::updateScreen();
    if (m_chocobo == nullptr || m_chocobo->isDead)
    {
        closeScreen();
        return;
    }
    if (m_renaming && m_nameField != nullptr)
        m_nameField->updateCursorCounter();
}

void GuiChocoboMenu::refreshJavaButtons()
{
    if (m_chocobo == nullptr)
        return;
    const bool es = chocoMenuIsSpanish();
    const char *aiStr = "";
    switch (m_chocobo->getAiMode())
    {
    case ChocoboAiMode::Wander: aiStr = es ? "Libre" : "Wander"; break;
    case ChocoboAiMode::Follow: aiStr = es ? "Seguir" : "Follow"; break;
    case ChocoboAiMode::Stay:   aiStr = es ? "Quieto" : "Stay"; break;
    }

    for (GuiButton *btn : controlList)
    {
        if (btn == nullptr) continue;
        if (btn->id == 0)
            btn->displayString = (es ? "Poner nombre: " : "Rename: ") +
                (m_chocobo->getCustomName().empty() ? (es ? "(Sin nombre)" : "(Unnamed)") : m_chocobo->getCustomName());
        else if (btn->id == 1)
        {
            btn->enabled = m_chocobo->hasSaddlebags();
            btn->displayString = m_chocobo->hasSaddlebags()
                ? (es ? "Abrir Alforjas (27 huecos)" : "Open Saddlebags (27 slots)")
                : (es ? "Alforjas (Sin equipar)" : "Saddlebags (Not equipped)");
        }
        else if (btn->id == 2)
            btn->displayString = std::string(es ? "Modo: " : "Behavior: ") + aiStr;
        else if (btn->id == 3)
        {
            btn->enabled = m_chocobo->isSaddled() && !m_chocobo->isChild();
            btn->displayString = m_chocobo->isSaddled()
                ? (es ? "Montar Chocobo" : "Mount Chocobo")
                : (es ? "Montar (Necesita silla)" : "Mount (Needs saddle)");
        }
    }
}

void GuiChocoboMenu::beginRename()
{
    if (m_nameField == nullptr || m_chocobo == nullptr)
        return;
    m_renaming = true;
    m_nameField->setText(m_chocobo->getCustomName());
    m_nameField->setFocused(true);
    if (mc != nullptr && mc->sndManager != nullptr)
        mc->sndManager->playSoundFX("random.click", 1.0f, 1.0f);
}

void GuiChocoboMenu::commitRename()
{
    if (m_chocobo != nullptr && m_nameField != nullptr)
    {
        m_chocobo->setCustomName(m_nameField->getText());
        refreshJavaButtons();
        if (mc != nullptr && mc->sndManager != nullptr)
            mc->sndManager->playSoundFX("random.orb", 1.0f, 1.1f);
    }
    cancelRename();
}

void GuiChocoboMenu::cancelRename()
{
    m_renaming = false;
    m_openLatch = true;
    if (m_nameField != nullptr)
        m_nameField->setFocused(false);
#if defined(PS2_PLATFORM) || defined(WII_PLATFORM) || defined(XBOX_PLATFORM)
    VirtualKeyboard::instance().releaseFocus();
#endif
}

void GuiChocoboMenu::openSaddlebags()
{
    if (m_chocobo == nullptr || m_player == nullptr || !m_chocobo->hasSaddlebags() || mc == nullptr)
        return;
    GuiChest *chest = new GuiChest(m_player->inventory, m_chocobo->getSaddlebagInventory(), m_player);
    if (mc->isSplitScreenActive() && m_playerIndex == 1)
        mc->displayPlayerScreen(1, chest);
    else
        mc->displayGuiScreen(chest);
}

void GuiChocoboMenu::activateRow(int_t row)
{
    if (m_chocobo == nullptr)
        return;
    if (row == 0)
    {
        beginRename();
    }
    else if (row == 1)
    {
        if (m_chocobo->hasSaddlebags())
            openSaddlebags();
    }
    else if (row == 2)
    {
        int next = (static_cast<int>(m_chocobo->getAiMode()) + 1) % 3;
        m_chocobo->setAiMode(static_cast<ChocoboAiMode>(next));
        refreshJavaButtons();
        if (mc != nullptr && mc->sndManager != nullptr)
            mc->sndManager->playSoundFX("random.click", 1.0f, 1.0f);
    }
    else if (row == 3)
    {
        if (m_chocobo->isSaddled() && !m_chocobo->isChild() && m_player != nullptr)
        {
            EntityPlayer *p = m_player;
            EntityChocobo *c = m_chocobo;
            closeScreen();
            p->mountEntity(c);
        }
    }
}

void GuiChocoboMenu::actionPerformed(GuiButton *button)
{
    if (button == nullptr || !button->enabled)
        return;
    if (button->id == 200)
        closeScreen();
    else if (button->id >= 0 && button->id <= 3)
        activateRow(button->id);
}

void GuiChocoboMenu::closeScreen()
{
    cancelRename();
    if (mc != nullptr)
    {
        if (mc->sndManager != nullptr)
            mc->sndManager->playSoundFX("random.back", 1.0f, 1.0f);
        if (mc->isSplitScreenActive() && m_playerIndex == 1)
            mc->closePlayerScreen(1);
        else
        {
            mc->displayGuiScreen(nullptr);
            mc->setIngameFocus();
        }
    }
}

void GuiChocoboMenu::handleSpecializedMenuInput()
{
    if (m_renaming)
        return;
#if PLATFORM_PS2 || PLATFORM_WII || PLATFORM_XBOX
    const PlatformTextInputSnapshot pad = platformTextInputSnapshot(platformMenuPad());
    if (!pad.connected)
        return;

    if (m_openLatch)
    {
        if ((pad.held & (PLATFORM_TEXT_TYPE | PLATFORM_TEXT_CLOSE | PLATFORM_TEXT_BACK | PLATFORM_TEXT_SHIFT)) == 0)
            m_openLatch = false;
        return;
    }

    if (pad.pressed & PLATFORM_TEXT_CLOSE)
    {
        closeScreen();
        return;
    }
    if (pad.pressed & PLATFORM_TEXT_UP)
    {
        m_selectedIndex = (m_selectedIndex + 3) % 4;
        if (mc != nullptr && mc->sndManager != nullptr)
            mc->sndManager->playSoundFX("random.focus", 1.0f, 1.0f);
    }
    else if (pad.pressed & PLATFORM_TEXT_DOWN)
    {
        m_selectedIndex = (m_selectedIndex + 1) % 4;
        if (mc != nullptr && mc->sndManager != nullptr)
            mc->sndManager->playSoundFX("random.focus", 1.0f, 1.0f);
    }
    else if (pad.pressed & PLATFORM_TEXT_TYPE)
    {
        activateRow(m_selectedIndex);
    }
#endif
}

void GuiChocoboMenu::keyTyped(char_t c, int_t key)
{
    if (m_renaming && m_nameField != nullptr)
    {
        if (key == 1)
        {
            cancelRename();
            return;
        }
        if (key == 28 || c == '\r' || c == '\n')
        {
            commitRename();
            return;
        }
        m_nameField->textboxKeyTyped(c, key);
        return;
    }

    if (key == 1)
    {
        closeScreen();
        return;
    }
#if !PLATFORM_PS2 && !PLATFORM_WII && !PLATFORM_XBOX
    if (key == lwjgl::Keyboard::KEY_UP) { m_selectedIndex = (m_selectedIndex + 3) % 4; return; }
    if (key == lwjgl::Keyboard::KEY_DOWN) { m_selectedIndex = (m_selectedIndex + 1) % 4; return; }
    if (key == lwjgl::Keyboard::KEY_RETURN || key == lwjgl::Keyboard::KEY_SPACE) { activateRow(m_selectedIndex); return; }
#endif
    GuiScreen::keyTyped(c, key);
}

void GuiChocoboMenu::drawScreen(int_t mouseX, int_t mouseY, float_t partialTick)
{
    if (m_chocobo == nullptr)
        return;

    const bool es = chocoMenuIsSpanish();
    const bool legacy = isLegacyMode();
    const bool isSplit = (mc != nullptr && mc->isSplitScreenActive());

    drawGradientRect(0, 0, width, height, static_cast<int_t>(0xC0080808u), static_cast<int_t>(0xE0141414u));

    const int_t hintY = legacyHintRowY(height, isSplit);
    const int_t panelW = std::min<int_t>(264, width - 16);
    const int_t panelH = std::min<int_t>(168, hintY - 14);
    const int_t panelX = (width - panelW) / 2;
    const int_t panelY = std::max<int_t>(10, (hintY - panelH) / 2);

    if (legacy)
    {
        LegacyOptionsLayout layout{};
        layout.panelX = panelX;
        layout.panelY = panelY;
        layout.panelWidth = panelW;
        layout.panelHeight = panelH;
        LegacyOptionsPanel panel;
        panel.draw(layout);
    }
    else
    {
        drawRect(panelX, panelY, panelX + panelW, panelY + panelH, static_cast<int_t>(0xD8141414u));
        drawRect(panelX, panelY, panelX + panelW, panelY + 1, static_cast<int_t>(0xFF666666u));
        drawRect(panelX, panelY + panelH - 1, panelX + panelW, panelY + panelH, static_cast<int_t>(0xFF666666u));
    }

    // Header & Chocobo Stats
    std::string title = m_chocobo->getCustomName().empty()
        ? (std::string("Chocobo ") + EntityChocobo::getColorName(m_chocobo->getColor(), es))
        : m_chocobo->getCustomName();
    drawCenteredString(fontRenderer, title, panelX + panelW / 2, panelY + 7, 0xFFE844);

    char statBuf[96];
    std::snprintf(statBuf, sizeof(statBuf), "%s | HP: %d/%d | %s",
        EntityChocobo::getColorName(m_chocobo->getColor(), es),
        static_cast<int>(m_chocobo->getHealth()),
        static_cast<int>(m_chocobo->getMaxHealth()),
        EntityChocobo::getColorAbility(m_chocobo->getColor(), es));
    drawCenteredString(fontRenderer, statBuf, panelX + panelW / 2, panelY + 20, 0xCCCCCC);

    if (legacy)
    {
        const char *aiStr = "";
        switch (m_chocobo->getAiMode())
        {
        case ChocoboAiMode::Wander: aiStr = es ? "Libre" : "Wander"; break;
        case ChocoboAiMode::Follow: aiStr = es ? "Seguir" : "Follow"; break;
        case ChocoboAiMode::Stay:   aiStr = es ? "Quieto" : "Stay"; break;
        }
        const std::string rows[4] = {
            std::string(es ? "Cambiar nombre: " : "Rename: ") +
                (m_chocobo->getCustomName().empty() ? (es ? "(Sin nombre)" : "(Unnamed)") : m_chocobo->getCustomName()),
            m_chocobo->hasSaddlebags()
                ? (es ? "Abrir Alforjas (27 huecos)" : "Open Saddlebags (27 slots)")
                : (es ? "Alforjas (Usa alforjas en el)" : "Saddlebags (Not equipped)"),
            std::string(es ? "Comportamiento: " : "Behavior: ") + aiStr,
            m_chocobo->isSaddled()
                ? (es ? "Montar Chocobo" : "Mount Chocobo")
                : (es ? "Montar (Equipa una silla)" : "Mount (Needs saddle)")
        };

        const int_t listX = panelX + 10;
        const int_t listW = panelW - 20;
        const int_t listY = panelY + 38;
        const int_t rowH = 24;
        for (int_t i = 0; i < 4; ++i)
        {
            const int_t ry = listY + i * rowH;
            const bool sel = (i == m_selectedIndex);
            const bool avail = (i == 0 || i == 2) ||
                               (i == 1 && m_chocobo->hasSaddlebags()) ||
                               (i == 3 && m_chocobo->isSaddled() && !m_chocobo->isChild());
            drawRect(listX, ry, listX + listW, ry + rowH - 3,
                sel ? static_cast<int_t>(0xD83A5288u) : static_cast<int_t>(0x90222222u));
            if (sel)
                drawRect(listX, ry, listX + 3, ry + rowH - 3, static_cast<int_t>(0xFFFFDD44u));
            drawString(fontRenderer, rows[i], listX + 8, ry + 6,
                avail ? (sel ? 0xFFFFAA : 0xFFFFFF) : 0x777777);
        }

        drawCenteredString(fontRenderer,
            es ? "Montado: A=Saltar/Volar(Dorado) | L3=Bajar"
               : "Mounted: A=Jump/Fly(Gold) | L3=Dismount",
            panelX + panelW / 2, panelY + panelH - 16, 0xAAAAAA);
    }
    else
    {
        GuiScreen::drawScreen(mouseX, mouseY, partialTick);
    }

    if (m_renaming && m_nameField != nullptr)
    {
        const int_t bx = (width - 208) / 2;
        const int_t by = std::max<int_t>(12, height / 2 - 70);
        drawRect(bx - 2, by - 2, bx + 210, by + 56, static_cast<int_t>(0xFFAAAAAAu));
        drawRect(bx, by, bx + 208, by + 54, static_cast<int_t>(0xF0181818u));
        drawCenteredString(fontRenderer, es ? "Nombre del Chocobo (Start: Guardar)" : "Chocobo Name (Start: Save)",
            width / 2, by + 6, 0xFFFFAA);
        m_nameField->drawTextBox();
    }

#if PLATFORM_XBOX
    const std::string btns[] = {"D-Pad", "A", "B"};
    const std::string acts[] = {
        es ? "Navegar" : "Navigate",
        es ? "Elegir" : "Select",
        es ? "Cerrar" : "Back"
    };
    drawControlHintRow(mc, width, hintY, btns, acts, 3);
#endif
}
