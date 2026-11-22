#include "GuiHerobrineMenu.h"
#include "legacy/LegacySceneState.h"
#include "HerobrineMod.h"

#include "net/minecraft/src/Minecraft.h"
#include "net/minecraft/src/GameSettings.h"
#include "net/minecraft/src/GuiButton.h"
#include "net/minecraft/src/FontRenderer.h"
#include "net/minecraft/src/SoundManager.h"
#include "net/minecraft/src/StringTranslate.h"
#include "net/minecraft/src/ControlIcon.h"
#include "net/minecraft/src/legacy/LegacyMenuHints.h"
#include "net/minecraft/src/legacy/LegacyOptionsLayout.h"
#include "net/minecraft/src/legacy/LegacyOptionsPanel.h"
#include "net/minecraft/src/legacy/LegacyPanorama.h"
#include "platform/PlatformConfig.h"
#include "platform/Input.h"

#if !PLATFORM_PS2 && !PLATFORM_WII && !PLATFORM_XBOX
#include "pc/lwjgl/Keyboard.h"
#endif

#include <algorithm>

namespace
{
bool hbMenuIsSpanish()
{
    StringTranslate *tr = StringTranslate::getInstance();
    return (tr != nullptr && tr->getCurrentLanguage().rfind("es_", 0) == 0);
}
}

GuiHerobrineMenu::GuiHerobrineMenu(GuiScreen *parent)
    : m_parent(parent)
    , m_selectedIndex(0)
    , m_scrollOffset(0)
    , m_openLatch(true)
{
}

bool GuiHerobrineMenu::isLegacyMode() const
{
    return mc != nullptr && mc->gameSettings != nullptr && mc->gameSettings->legacyUI;
}

bool GuiHerobrineMenu::usesSpecializedMenuNavigation() const
{
#if PLATFORM_PS2 || PLATFORM_WII || PLATFORM_XBOX
    return true;
#else
    return isLegacyMode();
#endif
}

bool GuiHerobrineMenu::isDestructiveItem(int_t idx) const
{
    return idx == 2 || idx == 3 || idx == 4 || idx == 5;
}

std::string GuiHerobrineMenu::getItemLabel(int_t idx) const
{
    const bool es = hbMenuIsSpanish();
    switch (idx)
    {
    case 0: return es ? "Ritual del altar" : "Altar Ritual";
    case 1: return es ? "Aparicion fantasma" : "Ghost Event";
    case 2: return es ? "Maldicion altar" : "Altar Curse";
    case 3: return es ? "Excavador" : "Tunnel Digger";
    case 4: return es ? "Trampa de diamantes" : "Diamond Lure";
    case 5: return es ? "Ladron de cofres" : "Chest Thief";
    case 6: return es ? "Extras de la leyenda" : "Legend Extras";
    default: return "";
    }
}

std::string GuiHerobrineMenu::getItemValue(int_t idx) const
{
    HerobrineMod *mod = HerobrineMod::getInstance();
    if (mod == nullptr)
        return "OFF";
    const HerobrineConfig &cfg = mod->getConfig();
    const bool es = hbMenuIsSpanish();
    auto onOff = [&](bool v) -> std::string { return v ? (es ? "SI" : "ON") : "OFF"; };
    switch (idx)
    {
    case 0: return onOff(cfg.altarRitual);
    case 1: return onOff(cfg.ghostEvent);
    case 2: return onOff(cfg.altarCurse);
    case 3: return onOff(cfg.diggerEvent);
    case 4: return onOff(cfg.diamondEvent);
    case 5: return onOff(cfg.thiefEvent);
    case 6: return onOff(cfg.legendExtras);
    default: return "OFF";
    }
}

std::string GuiHerobrineMenu::getItemDesc(int_t idx) const
{
    const bool es = hbMenuIsSpanish();
    switch (idx)
    {
    case 0: return es ? "Noche, apaga antorchas y aparece en el altar"
                      : "Nightfall, breaks torches & spawns on altar";
    case 1: return es ? "Observa a ~17 bloques con antorcha redstone"
                      : "Stares ~17 blocks away holding redstone torch";
    case 2: return es ? "AVISO: prende fuego al jugador y borra el altar"
                      : "WARN: sets player on fire & removes altar";
    case 3: return es ? "AVISO: cava tunel hacia sala secreta con creeper"
                      : "WARN: digs tunnel to secret underground room";
    case 4: return es ? "AVISO: cava pozo a diamantes y vuelve a taparlo"
                      : "WARN: digs shaft to diamonds then seals it";
    case 5: return es ? "AVISO: roba o tira objetos de cofres lejanos"
                      : "WARN: steals or drops items from distant chests";
    case 6: return es ? "Rayo en altar, sonidos, carteles y arboles secos"
                      : "Altar lightning, eerie sounds, signs & bare trees";
    default: return "";
    }
}

void GuiHerobrineMenu::toggleItem(int_t idx)
{
    HerobrineMod *mod = HerobrineMod::getInstance();
    if (mod == nullptr)
        return;
    HerobrineConfig &cfg = mod->getConfig();
    switch (idx)
    {
    case 0: cfg.altarRitual = !cfg.altarRitual; break;
    case 1: cfg.ghostEvent = !cfg.ghostEvent; break;
    case 2: cfg.altarCurse = !cfg.altarCurse; break;
    case 3: cfg.diggerEvent = !cfg.diggerEvent; break;
    case 4: cfg.diamondEvent = !cfg.diamondEvent; break;
    case 5: cfg.thiefEvent = !cfg.thiefEvent; break;
    case 6: cfg.legendExtras = !cfg.legendExtras; break;
    default: return;
    }
    mod->saveConfig();
    refreshJavaButtons();
    if (mc != nullptr && mc->sndManager != nullptr)
        mc->sndManager->playSoundFX("random.click", 1.0f, 1.0f);
}

void GuiHerobrineMenu::ensureSelectionVisible(int_t visibleRows)
{
    if (visibleRows < 1) visibleRows = 1;
    if (m_selectedIndex < m_scrollOffset)
        m_scrollOffset = m_selectedIndex;
    if (m_selectedIndex >= m_scrollOffset + visibleRows)
        m_scrollOffset = m_selectedIndex - visibleRows + 1;
    const int_t maxScroll = std::max<int_t>(0, getItemCount() - visibleRows);
    m_scrollOffset = std::clamp<int_t>(m_scrollOffset, 0, maxScroll);
}

void GuiHerobrineMenu::refreshJavaButtons()
{
    for (GuiButton *btn : controlList)
    {
        if (btn == nullptr) continue;
        if (btn->id >= 0 && btn->id < getItemCount())
            btn->displayString = getItemLabel(btn->id) + ": " + getItemValue(btn->id);
    }
}

void GuiHerobrineMenu::initGui()
{
    m_openLatch = true;
    controlList.clear();

    if (!isLegacyMode())
    {
        const bool es = hbMenuIsSpanish();
        const int_t colW = 148;
        const int_t gap = 8;
        const int_t leftX = width / 2 - colW - gap / 2;
        const int_t rightX = width / 2 + gap / 2;
        const int_t startY = 44;
        for (int_t i = 0; i < getItemCount(); ++i)
        {
            const int_t col = i % 2;
            const int_t row = i / 2;
            const int_t bx = (col == 0) ? leftX : rightX;
            controlList.push_back(new GuiButton(i, bx, startY + row * 24, colW, 20, ""));
        }
        controlList.push_back(new GuiButton(200, width / 2 - 100, height - 28, 200, 20, es ? "Hecho" : "Done"));
        refreshJavaButtons();
    }
}

void GuiHerobrineMenu::closeScreen()
{
    if (HerobrineMod *mod = HerobrineMod::getInstance())
        mod->saveConfig();
    if (mc != nullptr)
    {
        if (mc->sndManager != nullptr)
            mc->sndManager->playSoundFX("random.back", 1.0f, 1.0f);
        mc->displayGuiScreen(m_parent);
    }
}

void GuiHerobrineMenu::actionPerformed(GuiButton *button)
{
    if (button == nullptr || !button->enabled)
        return;
    if (button->id == 200)
        closeScreen();
    else if (button->id >= 0 && button->id < getItemCount())
        toggleItem(button->id);
}

void GuiHerobrineMenu::keyTyped(char_t c, int_t key)
{
    if (key == 1)
    {
        closeScreen();
        return;
    }
#if !PLATFORM_PS2 && !PLATFORM_WII && !PLATFORM_XBOX
    if (key == lwjgl::Keyboard::KEY_UP) { m_selectedIndex = (m_selectedIndex + getItemCount() - 1) % getItemCount(); return; }
    if (key == lwjgl::Keyboard::KEY_DOWN) { m_selectedIndex = (m_selectedIndex + 1) % getItemCount(); return; }
    if (key == lwjgl::Keyboard::KEY_RETURN || key == lwjgl::Keyboard::KEY_SPACE) { toggleItem(m_selectedIndex); return; }
#endif
    GuiScreen::keyTyped(c, key);
}

void GuiHerobrineMenu::handleSpecializedMenuInput()
{
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
        m_selectedIndex = (m_selectedIndex + getItemCount() - 1) % getItemCount();
        if (mc != nullptr && mc->sndManager != nullptr)
            mc->sndManager->playSoundFX("random.focus", 1.0f, 1.0f);
    }
    else if (pad.pressed & PLATFORM_TEXT_DOWN)
    {
        m_selectedIndex = (m_selectedIndex + 1) % getItemCount();
        if (mc != nullptr && mc->sndManager != nullptr)
            mc->sndManager->playSoundFX("random.focus", 1.0f, 1.0f);
    }
    else if (pad.pressed & PLATFORM_TEXT_TYPE)
    {
        toggleItem(m_selectedIndex);
    }
#endif
}

void GuiHerobrineMenu::drawScreen(int_t mouseX, int_t mouseY, float_t partialTick)
{
    const bool es = hbMenuIsSpanish();
    const bool isSplit = (mc != nullptr && mc->isSplitScreenActive());
    const std::string title = es ? "Herobrine Classic (iLoveYou)" : "Herobrine Classic (iLoveYou)";

    if (!isLegacyMode())
    {
        drawDefaultBackground();
        drawCenteredString(fontRenderer, title, width / 2, 12, 0xFFFFFF);
        drawCenteredString(fontRenderer,
            es ? "\xc2\xa7" "7Altar: 8 oro + musgosa + netherrack + 4 redstone"
               : "\xc2\xa7" "7Altar: 8 gold + mossy + netherrack + 4 redstone",
            width / 2, 26, 0xAAAAAA);
        GuiScreen::drawScreen(mouseX, mouseY, partialTick);
        drawCenteredString(fontRenderer, getItemDesc(m_selectedIndex), width / 2, height - 46,
            isDestructiveItem(m_selectedIndex) ? 0xFFAA66 : 0xCCCCCC);
        return;
    }

    const bool inWorld = (mc != nullptr && mc->theWorld != nullptr);
    if (!inWorld && mc != nullptr)
        legacyDrawPanorama(mc, width, height, legacyScenePanoramaTimer(), partialTick, zLevel);
    else
        drawGradientRect(0, 0, width, height, static_cast<int_t>(0xC0101010u), static_cast<int_t>(0xD8101010u));

    const int_t hintY = legacyHintRowY(height, isSplit);
    const int_t panelW = std::min<int_t>(292, width - 14);
    const int_t panelH = std::min<int_t>(182, hintY - 12);
    const int_t panelX = (width - panelW) / 2;
    const int_t panelY = std::max<int_t>(8, (hintY - panelH) / 2);

    LegacyOptionsLayout layout{};
    layout.panelX = panelX;
    layout.panelY = panelY;
    layout.panelWidth = panelW;
    layout.panelHeight = panelH;
    LegacyOptionsPanel panelRenderer;
    panelRenderer.draw(layout);

    drawCenteredString(fontRenderer, title, panelX + panelW / 2, panelY + 7, 0xFFFFFF);
    drawCenteredString(fontRenderer,
        es ? "Altar: 8 oro + musgosa + netherrack + 4 redstone"
           : "Altar: 8 gold + mossy + netherrack + 4 redstone",
        panelX + panelW / 2, panelY + 18, 0xFFDD88);
    drawRect(panelX + 8, panelY + 29, panelX + panelW - 8, panelY + 30, static_cast<int_t>(0xFF777777u));

    const int_t listX = panelX + 10;
    const int_t listY = panelY + 34;
    const int_t listW = panelW - 20;
    const int_t rowH = 18;
    const int_t availH = std::max<int_t>(rowH, panelH - 58);
    const int_t visibleRows = std::min<int_t>(getItemCount(), std::max<int_t>(1, availH / rowH));
    ensureSelectionVisible(visibleRows);

    for (int_t vi = 0; vi < visibleRows; ++vi)
    {
        const int_t idx = m_scrollOffset + vi;
        if (idx >= getItemCount())
            break;
        const int_t ry = listY + vi * rowH;
        const bool sel = (idx == m_selectedIndex);
        drawRect(listX, ry, listX + listW, ry + rowH - 2,
            sel ? static_cast<int_t>(0xD83A5288u) : static_cast<int_t>(0x90222222u));
        if (sel)
            drawRect(listX, ry, listX + 3, ry + rowH - 2, static_cast<int_t>(0xFFFFDD44u));

        const int_t labelColor = sel ? 0xFFFFAA : (isDestructiveItem(idx) ? 0xFFCC99 : 0xFFFFFF);
        drawString(fontRenderer, getItemLabel(idx), listX + 7, ry + 4, labelColor);
        std::string val = getItemValue(idx);
        drawString(fontRenderer, val, listX + listW - fontRenderer->getStringWidth(val) - 6, ry + 4,
            val == "OFF" ? 0xFF8888 : 0xAAFFAA);
    }

    drawCenteredString(fontRenderer, getItemDesc(m_selectedIndex),
        panelX + panelW / 2, panelY + panelH - 16,
        isDestructiveItem(m_selectedIndex) ? 0xFFAA66 : 0xCCCCCC);

#if PLATFORM_XBOX
    const std::string btns[] = {"D-Pad", "A", "B"};
    const std::string acts[] = {
        es ? "Navegar" : "Navigate",
        es ? "Cambiar" : "Toggle",
        es ? "Volver" : "Back"
    };
    drawControlHintRow(mc, width, hintY, btns, acts, 3);
#endif
}
