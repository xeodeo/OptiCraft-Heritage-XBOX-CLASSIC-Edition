#include "GuiMinimapMenu.h"
#include "legacy/LegacySceneState.h"
#include "GuiWaypointManager.h"
#include "ReiMinimap.h"

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
#include <cstdio>

namespace
{
bool minimapMenuIsSpanish()
{
    StringTranslate *tr = StringTranslate::getInstance();
    return (tr != nullptr && tr->getCurrentLanguage().rfind("es_", 0) == 0);
}
}

GuiMinimapMenu::GuiMinimapMenu(GuiScreen *parent, int playerIndex)
    : m_parent(parent)
    , m_playerIndex(playerIndex)
    , m_selectedIndex(0)
    , m_scrollOffset(0)
    , m_openLatch(true)
{
}

bool GuiMinimapMenu::isLegacyMode() const
{
    return mc != nullptr && mc->gameSettings != nullptr && mc->gameSettings->legacyUI;
}

bool GuiMinimapMenu::usesSpecializedMenuNavigation() const
{
#if PLATFORM_PS2 || PLATFORM_WII || PLATFORM_XBOX
    return true;
#else
    return isLegacyMode();
#endif
}

int_t GuiMinimapMenu::getItemCount() const
{
    const bool inWorld = (mc != nullptr && mc->theWorld != nullptr);
    return inWorld ? 9 : 8;
}

std::string GuiMinimapMenu::getItemLabel(int_t index) const
{
    const bool es = minimapMenuIsSpanish();
    switch (index)
    {
    case 0: return es ? "Forma" : "Shape";
    case 1: return es ? "Zoom" : "Zoom";
    case 2: return es ? "Radar de entidades" : "Entity Radar";
    case 3: return es ? "Modo cueva" : "Cave Mode";
    case 4: return es ? "Rotar mapa" : "Rotate Map";
    case 5: return es ? "Coordenadas" : "Coordinates";
    case 6: return es ? "Mostrar bioma" : "Show Biome";
    case 7: return es ? "Puntos de muerte" : "Death Points";
    case 8: return es ? "Gestor de Waypoints..." : "Waypoint Manager...";
    default: return "";
    }
}

std::string GuiMinimapMenu::getItemValue(int_t index) const
{
    const bool es = minimapMenuIsSpanish();
    const MinimapSettings &s = ReiMinimap::getInstance().getSettings();
    const char *onStr = es ? "SI" : "ON";
    const char *offStr = es ? "NO" : "OFF";
    switch (index)
    {
    case 0: return s.shape == MinimapShape::Round ? (es ? "Redondo" : "Round") : (es ? "Cuadrado" : "Square");
    case 1: return ReiMinimap::getInstance().getZoomLabel();
    case 2: return s.entityRadar ? onStr : offStr;
    case 3:
        if (s.caveMode == MinimapCaveMode::Off) return offStr;
        if (s.caveMode == MinimapCaveMode::Auto) return "Auto";
        return es ? "Siempre" : "Always";
    case 4: return s.rotateMap ? onStr : offStr;
    case 5: return s.showCoords ? onStr : offStr;
    case 6: return s.showBiome ? onStr : offStr;
    case 7: return s.deathWaypoints ? onStr : offStr;
    case 8: return std::to_string(ReiMinimap::getInstance().getWaypointCount());
    default: return "";
    }
}

void GuiMinimapMenu::activateItem(int_t index, int_t dir)
{
    ReiMinimap &mm = ReiMinimap::getInstance();
    MinimapSettings &s = mm.getSettings();
    switch (index)
    {
    case 0:
        s.shape = (s.shape == MinimapShape::Round) ? MinimapShape::Square : MinimapShape::Round;
        break;
    case 1:
        s.zoomIndex = (s.zoomIndex + (dir >= 0 ? 1 : 3)) % 4;
        mm.markMapDirty();
        break;
    case 2:
        s.entityRadar = !s.entityRadar;
        break;
    case 3:
        s.caveMode = static_cast<MinimapCaveMode>((static_cast<int>(s.caveMode) + (dir >= 0 ? 1 : 2)) % 3);
        mm.markMapDirty();
        break;
    case 4:
        s.rotateMap = !s.rotateMap;
        break;
    case 5:
        s.showCoords = !s.showCoords;
        break;
    case 6:
        s.showBiome = !s.showBiome;
        break;
    case 7:
        s.deathWaypoints = !s.deathWaypoints;
        break;
    case 8:
        mm.saveSettings();
        if (mc != nullptr)
        {
            if (mc->sndManager != nullptr)
                mc->sndManager->playSoundFX("random.click", 1.0f, 1.0f);
            if (mc->isSplitScreenActive() && m_playerIndex == 1)
                mc->displayPlayerScreen(1, new GuiWaypointManager(1));
            else
                mc->displayGuiScreen(new GuiWaypointManager(m_playerIndex));
        }
        return;
    default:
        return;
    }

    mm.saveSettings();
    refreshJavaButtons();
    if (mc != nullptr && mc->sndManager != nullptr)
        mc->sndManager->playSoundFX("random.click", 1.0f, 1.0f);
}

void GuiMinimapMenu::refreshJavaButtons()
{
    for (GuiButton *btn : controlList)
    {
        if (btn == nullptr)
            continue;
        if (btn->id >= 0 && btn->id < getItemCount())
        {
            if (btn->id == 8)
                btn->displayString = getItemLabel(8) + " (" + getItemValue(8) + ")";
            else
                btn->displayString = getItemLabel(btn->id) + ": " + getItemValue(btn->id);
        }
    }
}

void GuiMinimapMenu::initGui()
{
    m_openLatch = true;
    controlList.clear();

    if (!isLegacyMode())
    {
        const int_t count = getItemCount();
        const int_t startY = height / 6 + 6;
        for (int_t i = 0; i < count; ++i)
        {
            const int_t bx = (width / 2 - 155) + (i % 2) * 160;
            const int_t by = startY + 24 * (i / 2);
            const int_t bw = (i == 8) ? 310 : 150;
            controlList.push_back(new GuiButton(i, (i == 8) ? (width / 2 - 155) : bx, by, bw, 20, ""));
        }
        StringTranslate *tr = StringTranslate::getInstance();
        controlList.push_back(new GuiButton(200, width / 2 - 100, height - 28, tr->translateKey("gui.done")));
        refreshJavaButtons();
    }
}

void GuiMinimapMenu::actionPerformed(GuiButton *button)
{
    if (button == nullptr || !button->enabled)
        return;
    if (button->id == 200)
    {
        closeScreen();
        return;
    }
    if (button->id >= 0 && button->id < getItemCount())
    {
        m_selectedIndex = button->id;
        activateItem(button->id, 1);
    }
}

void GuiMinimapMenu::closeScreen()
{
    ReiMinimap::getInstance().saveSettings();
    if (mc != nullptr)
    {
        if (mc->sndManager != nullptr)
            mc->sndManager->playSoundFX("random.back", 1.0f, 1.0f);
        if (m_parent != nullptr)
        {
            mc->displayGuiScreen(m_parent);
        }
        else if (mc->isSplitScreenActive() && m_playerIndex == 1)
        {
            mc->closePlayerScreen(1);
        }
        else
        {
            mc->displayGuiScreen(nullptr);
            mc->setIngameFocus();
        }
    }
}

void GuiMinimapMenu::handleSpecializedMenuInput()
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

    const int_t count = getItemCount();
    if (pad.pressed & PLATFORM_TEXT_UP)
    {
        m_selectedIndex = (m_selectedIndex - 1 + count) % count;
        if (mc != nullptr && mc->sndManager != nullptr)
            mc->sndManager->playSoundFX("random.focus", 1.0f, 1.0f);
    }
    else if (pad.pressed & PLATFORM_TEXT_DOWN)
    {
        m_selectedIndex = (m_selectedIndex + 1) % count;
        if (mc != nullptr && mc->sndManager != nullptr)
            mc->sndManager->playSoundFX("random.focus", 1.0f, 1.0f);
    }
    else if (pad.pressed & PLATFORM_TEXT_LEFT)
    {
        activateItem(m_selectedIndex, -1);
    }
    else if (pad.pressed & (PLATFORM_TEXT_RIGHT | PLATFORM_TEXT_TYPE))
    {
        activateItem(m_selectedIndex, 1);
    }
#endif
}

void GuiMinimapMenu::keyTyped(char_t c, int_t key)
{
    if (key == 1)
    {
        closeScreen();
        return;
    }
#if !PLATFORM_PS2 && !PLATFORM_WII && !PLATFORM_XBOX
    const int_t count = getItemCount();
    if (key == lwjgl::Keyboard::KEY_UP)
    {
        m_selectedIndex = (m_selectedIndex - 1 + count) % count;
        return;
    }
    if (key == lwjgl::Keyboard::KEY_DOWN)
    {
        m_selectedIndex = (m_selectedIndex + 1) % count;
        return;
    }
    if (key == lwjgl::Keyboard::KEY_LEFT)
    {
        activateItem(m_selectedIndex, -1);
        return;
    }
    if (key == lwjgl::Keyboard::KEY_RIGHT || key == lwjgl::Keyboard::KEY_RETURN || key == lwjgl::Keyboard::KEY_SPACE)
    {
        activateItem(m_selectedIndex, 1);
        return;
    }
#endif
    GuiScreen::keyTyped(c, key);
}

void GuiMinimapMenu::drawScreen(int_t mouseX, int_t mouseY, float_t partialTick)
{
    const bool es = minimapMenuIsSpanish();
    const bool isSplit = (mc != nullptr && mc->isSplitScreenActive());
    const std::string title = es ? "Opciones del Minimapa" : "Minimap Options";

    if (!isLegacyMode())
    {
        drawDefaultBackground();
        drawCenteredString(fontRenderer, title, width / 2, 14, 0xFFFFFF);
        drawCenteredString(fontRenderer,
            es ? "\xc2\xa7" "7Back+Cruceta: Arr=WP  Abj=Lista  Izq=Zoom  Der=Menu"
               : "\xc2\xa7" "7Back+D-Pad: Up=WP  Down=List  Left=Zoom  Right=Menu",
            width / 2, 26, 0xAAAAAA);
        GuiScreen::drawScreen(mouseX, mouseY, partialTick);
        return;
    }

    const bool inWorld = (mc != nullptr && mc->theWorld != nullptr);
    if (!inWorld && mc != nullptr && legacyDrawPanorama(mc, width, height, legacyScenePanoramaTimer(), partialTick, zLevel))
    {
    }
    else
        drawGradientRect(0, 0, width, height, static_cast<int_t>(0xC0101010u), static_cast<int_t>(0xD8101010u));

    const int_t hintY = legacyHintRowY(height, isSplit);
    const int_t panelW = std::min<int_t>(280, width - 16);
    const int_t panelY = std::max<int_t>(10, (height - 196) / 2 - 6);
    const int_t panelH = std::min<int_t>(184, hintY - panelY - 6);
    const int_t panelX = (width - panelW) / 2;

    LegacyOptionsLayout layout{};
    layout.panelX = panelX;
    layout.panelY = panelY;
    layout.panelWidth = panelW;
    layout.panelHeight = panelH;
    LegacyOptionsPanel panelRenderer;
    panelRenderer.draw(layout);

    drawCenteredString(fontRenderer, title, panelX + panelW / 2, panelY + 7, 0xFFFFFF);
    drawRect(panelX + 8, panelY + 19, panelX + panelW - 8, panelY + 20, static_cast<int_t>(0xFF777777u));

    const int_t count = getItemCount();
    const int_t listX = panelX + 10;
    const int_t listY = panelY + 24;
    const int_t listW = panelW - 20;
    const int_t rowH = 18;
    const int_t visibleRows = std::max<int_t>(1, (panelY + panelH - 8 - listY) / rowH);

    m_selectedIndex = std::clamp<int>(m_selectedIndex, 0, count - 1);
    if (m_selectedIndex < m_scrollOffset) m_scrollOffset = m_selectedIndex;
    if (m_selectedIndex >= m_scrollOffset + visibleRows) m_scrollOffset = m_selectedIndex - visibleRows + 1;
    const int_t maxScroll = std::max<int_t>(0, count - visibleRows);
    m_scrollOffset = std::clamp<int>(m_scrollOffset, 0, maxScroll);

    const int_t rowsToDraw = std::min<int_t>(visibleRows, count - m_scrollOffset);
    for (int_t r = 0; r < rowsToDraw; ++r)
    {
        const int_t idx = m_scrollOffset + r;
        const int_t ry = listY + r * rowH;
        const bool sel = (idx == m_selectedIndex);

        drawRect(listX, ry, listX + listW, ry + rowH - 2,
            sel ? static_cast<int_t>(0xD83A5288u) : static_cast<int_t>(0x90222222u));
        if (sel)
            drawRect(listX, ry, listX + 3, ry + rowH - 2, static_cast<int_t>(0xFFFFDD44u));

        drawString(fontRenderer, getItemLabel(idx), listX + 7, ry + 4, sel ? 0xFFFFAA : 0xFFFFFF);
        std::string val = getItemValue(idx);
        drawString(fontRenderer, val, listX + listW - fontRenderer->getStringWidth(val) - 6, ry + 4,
            sel ? 0xAAFFAA : 0xCCCCCC);
    }

#if PLATFORM_XBOX
    const std::string btns[] = {"D-Pad", "A", "B"};
    const std::string acts[] = {
        es ? "Navegar" : "Navigate",
        es ? "Cambiar" : "Toggle",
        es ? "Volver" : "Back"
    };
    drawControlHintRow(mc, width, hintY, btns, acts, 3);
#else
    const std::string btns[] = {"Up/Down", "Enter", "Esc"};
    const std::string acts[] = {
        es ? "Navegar" : "Navigate",
        es ? "Cambiar" : "Toggle",
        es ? "Volver" : "Back"
    };
    drawControlHintRow(mc, width, hintY, btns, acts, 3);
#endif
}
