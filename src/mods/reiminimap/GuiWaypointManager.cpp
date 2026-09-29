#include "GuiWaypointManager.h"
#include "ReiMinimap.h"
#include "Waypoint.h"

#include "net/minecraft/src/Minecraft.h"
#include "net/minecraft/src/FontRenderer.h"
#include "net/minecraft/src/SoundManager.h"
#include "net/minecraft/src/StringTranslate.h"
#include "net/minecraft/src/UiStrings.h"
#include "net/minecraft/src/legacy/LegacyMenuHints.h"
#include "net/minecraft/src/ControlIcon.h"
#include "platform/PlatformConfig.h"
#include "platform/RenderAPI.h"
#include "platform/Input.h"

#if PLATFORM_PS2
#include "ps2/input/Ps2PadState.h"
#endif

#if !PLATFORM_PS2 && !PLATFORM_WII && !PLATFORM_XBOX
#include "pc/lwjgl/Keyboard.h"
#endif

#include <algorithm>
#include <cstdio>

static bool isSpanishLocale()
{
    StringTranslate *tr = StringTranslate::getInstance();
    return (tr != nullptr && tr->getCurrentLanguage().rfind("es_", 0) == 0);
}

GuiWaypointManager::GuiWaypointManager(int playerIndex)
    : m_playerIndex(playerIndex)
    , m_selectedIndex(0)
    , m_scrollOffset(0)
    , m_maxVisibleRows(6)
    , m_triangleLatch(true)
    , m_downLatch(true)
    , m_actionLatch(true)
    , m_squareLatch(true)
    , m_circleLatch(true)
    , m_prevHeld(0)
    , m_prevLeftY(128)
{
}

void GuiWaypointManager::initGui()
{
    m_selectedIndex = 0;
    m_scrollOffset = 0;
    m_prevHeld = 0;
    m_prevLeftY = 128;
    m_triangleLatch = true;
    m_downLatch = true;
    m_actionLatch = true;
    m_squareLatch = true;
    m_circleLatch = true;
}

void GuiWaypointManager::updateScreen()
{
    GuiScreen::updateScreen();
}

void GuiWaypointManager::handleSpecializedMenuInput()
{
#if PLATFORM_PS2
    const Ps2PadSnapshot &pad = ps2PadGetSnapshot(m_playerIndex);
    if (!pad.connected)
        return;

    // Release opening latches so holding Triangle or Down doesn't trigger immediately
    if (m_triangleLatch && (pad.held & PS2_PAD_TRIANGLE) == 0)
        m_triangleLatch = false;
    if (m_downLatch && (pad.held & PS2_PAD_DOWN) == 0)
        m_downLatch = false;
    if (m_actionLatch && (pad.held & PS2_PAD_CROSS) == 0)
        m_actionLatch = false;
    if (m_squareLatch && (pad.held & PS2_PAD_SQUARE) == 0)
        m_squareLatch = false;
    if (m_circleLatch && (pad.held & PS2_PAD_CIRCLE) == 0)
        m_circleLatch = false;

    const unsigned short pressed = static_cast<unsigned short>(pad.held & ~m_prevHeld);
    m_prevHeld = pad.held;

    // Circle or Triangle to close
    if ((!m_circleLatch && (pressed & PS2_PAD_CIRCLE)) ||
        (!m_triangleLatch && (pressed & PS2_PAD_TRIANGLE)))
    {
        closeScreen();
        return;
    }

    const size_t totalWaypoints = ReiMinimap::getInstance().getWaypointCount();

    // Up navigation
    if ((pressed & PS2_PAD_UP) || (pad.leftY < 64 && m_prevLeftY >= 64))
    {
        if (totalWaypoints > 0)
        {
            if (m_selectedIndex > 0)
                m_selectedIndex--;
            else
                m_selectedIndex = static_cast<int>(totalWaypoints) - 1;

            if (mc != nullptr && mc->sndManager != nullptr)
                mc->sndManager->playSoundFX("random.focus", 1.0f, 1.0f);
        }
    }
    // Down navigation
    else if ((!m_downLatch && (pressed & PS2_PAD_DOWN)) || (pad.leftY > 192 && m_prevLeftY <= 192))
    {
        if (totalWaypoints > 0)
        {
            if (m_selectedIndex + 1 < static_cast<int>(totalWaypoints))
                m_selectedIndex++;
            else
                m_selectedIndex = 0;

            if (mc != nullptr && mc->sndManager != nullptr)
                mc->sndManager->playSoundFX("random.focus", 1.0f, 1.0f);
        }
    }
    m_prevLeftY = pad.leftY;

    // Cross: Toggle visibility
    if (!m_actionLatch && (pressed & PS2_PAD_CROSS))
    {
        if (totalWaypoints > 0 && m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(totalWaypoints))
        {
            ReiMinimap::getInstance().toggleWaypoint(static_cast<size_t>(m_selectedIndex));
            if (mc != nullptr && mc->sndManager != nullptr)
                mc->sndManager->playSoundFX("random.click", 1.0f, 1.0f);
        }
    }

    // Square: Delete waypoint
    if (!m_squareLatch && (pressed & PS2_PAD_SQUARE))
    {
        if (totalWaypoints > 0 && m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(totalWaypoints))
        {
            ReiMinimap::getInstance().removeWaypoint(static_cast<size_t>(m_selectedIndex));
            const size_t newTotal = ReiMinimap::getInstance().getWaypointCount();
            if (m_selectedIndex >= static_cast<int>(newTotal) && newTotal > 0)
                m_selectedIndex = static_cast<int>(newTotal) - 1;
            if (mc != nullptr && mc->sndManager != nullptr)
                mc->sndManager->playSoundFX("random.pop", 1.0f, 1.0f);
        }
    }
#elif PLATFORM_XBOX
    const PlatformTextInputSnapshot pad = platformTextInputSnapshot(platformMenuPad());
    if (!pad.connected)
        return;

    if (pad.pressed & (PLATFORM_TEXT_CLOSE | PLATFORM_TEXT_SHIFT))
    {
        closeScreen();
        return;
    }

    const size_t totalWaypoints = ReiMinimap::getInstance().getWaypointCount();

    if (pad.pressed & PLATFORM_TEXT_UP)
    {
        if (totalWaypoints > 0)
        {
            if (m_selectedIndex > 0)
                m_selectedIndex--;
            else
                m_selectedIndex = static_cast<int>(totalWaypoints) - 1;

            if (mc != nullptr && mc->sndManager != nullptr)
                mc->sndManager->playSoundFX("random.focus", 1.0f, 1.0f);
        }
    }
    else if (pad.pressed & PLATFORM_TEXT_DOWN)
    {
        if (totalWaypoints > 0)
        {
            if (m_selectedIndex + 1 < static_cast<int>(totalWaypoints))
                m_selectedIndex++;
            else
                m_selectedIndex = 0;

            if (mc != nullptr && mc->sndManager != nullptr)
                mc->sndManager->playSoundFX("random.focus", 1.0f, 1.0f);
        }
    }

    // A: Toggle visibility (PLATFORM_TEXT_TYPE)
    if (pad.pressed & PLATFORM_TEXT_TYPE)
    {
        if (totalWaypoints > 0 && m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(totalWaypoints))
        {
            ReiMinimap::getInstance().toggleWaypoint(static_cast<size_t>(m_selectedIndex));
            if (mc != nullptr && mc->sndManager != nullptr)
                mc->sndManager->playSoundFX("random.click", 1.0f, 1.0f);
        }
    }

    // X (BACK): Delete waypoint
    if (pad.pressed & PLATFORM_TEXT_BACK)
    {
        if (totalWaypoints > 0 && m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(totalWaypoints))
        {
            ReiMinimap::getInstance().removeWaypoint(static_cast<size_t>(m_selectedIndex));
            const size_t newTotal = ReiMinimap::getInstance().getWaypointCount();
            if (m_selectedIndex >= static_cast<int>(newTotal) && newTotal > 0)
                m_selectedIndex = static_cast<int>(newTotal) - 1;
            if (mc != nullptr && mc->sndManager != nullptr)
                mc->sndManager->playSoundFX("random.pop", 1.0f, 1.0f);
        }
    }
#endif
}

void GuiWaypointManager::keyTyped(char_t c, int_t key)
{
#if !PLATFORM_PS2 && !PLATFORM_WII && !PLATFORM_XBOX
    const size_t totalWaypoints = ReiMinimap::getInstance().getWaypointCount();
    if (key == lwjgl::Keyboard::KEY_UP)
    {
        if (totalWaypoints > 0)
        {
            if (m_selectedIndex > 0)
                m_selectedIndex--;
            else
                m_selectedIndex = static_cast<int>(totalWaypoints) - 1;
            if (mc != nullptr && mc->sndManager != nullptr)
                mc->sndManager->playSoundFX("random.focus", 1.0f, 1.0f);
        }
        return;
    }
    if (key == lwjgl::Keyboard::KEY_DOWN)
    {
        if (totalWaypoints > 0)
        {
            if (m_selectedIndex + 1 < static_cast<int>(totalWaypoints))
                m_selectedIndex++;
            else
                m_selectedIndex = 0;
            if (mc != nullptr && mc->sndManager != nullptr)
                mc->sndManager->playSoundFX("random.focus", 1.0f, 1.0f);
        }
        return;
    }
    if (key == lwjgl::Keyboard::KEY_SPACE || key == lwjgl::Keyboard::KEY_RETURN)
    {
        if (totalWaypoints > 0 && m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(totalWaypoints))
        {
            ReiMinimap::getInstance().toggleWaypoint(static_cast<size_t>(m_selectedIndex));
            if (mc != nullptr && mc->sndManager != nullptr)
                mc->sndManager->playSoundFX("random.click", 1.0f, 1.0f);
        }
        return;
    }
    if (key == lwjgl::Keyboard::KEY_DELETE || key == lwjgl::Keyboard::KEY_BACK)
    {
        if (totalWaypoints > 0 && m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(totalWaypoints))
        {
            ReiMinimap::getInstance().removeWaypoint(static_cast<size_t>(m_selectedIndex));
            const size_t newTotal = ReiMinimap::getInstance().getWaypointCount();
            if (m_selectedIndex >= static_cast<int>(newTotal) && newTotal > 0)
                m_selectedIndex = static_cast<int>(newTotal) - 1;
            if (mc != nullptr && mc->sndManager != nullptr)
                mc->sndManager->playSoundFX("random.pop", 1.0f, 1.0f);
        }
        return;
    }
    if (key == lwjgl::Keyboard::KEY_ESCAPE)
    {
        closeScreen();
        return;
    }
#endif
    GuiScreen::keyTyped(c, key);
}

void GuiWaypointManager::closeScreen()
{
    if (mc != nullptr)
    {
        if (mc->sndManager != nullptr)
            mc->sndManager->playSoundFX("random.back", 1.0f, 1.0f);
        if (mc->isSplitScreenActive() && m_playerIndex >= 0)
        {
            mc->closePlayerScreen(m_playerIndex);
        }
        else
        {
            mc->displayGuiScreen(nullptr);
            mc->setIngameFocus();
        }
    }
}

void GuiWaypointManager::drawScreen(int_t mouseX, int_t mouseY, float_t partialTick)
{
    const bool isEs = isSpanishLocale();
    const bool isSplit = (mc != nullptr && mc->isSplitScreenActive());

    // 1. Dark translucent background overlay
    drawGradientRect(0, 0, width, height, 0xC0050505, 0xE0101010);

    // 2. Central Frame Card
    const int_t panelW = std::min<int_t>(280, width - 20);
    const int_t panelH = std::min<int_t>(186, height - 32);
    const int_t panelX = (width - panelW) / 2;
    const int_t panelY = (height - panelH) / 2 - 4;

    // Card background & borders
    drawRect(panelX, panelY, panelX + panelW, panelY + panelH, 0xD0121212);
    drawRect(panelX, panelY, panelX + panelW, panelY + 1, 0xFF444444);
    drawRect(panelX, panelY + panelH - 1, panelX + panelW, panelY + panelH, 0xFF444444);
    drawRect(panelX, panelY, panelX + 1, panelY + panelH, 0xFF444444);
    drawRect(panelX + panelW - 1, panelY, panelX + panelW, panelY + panelH, 0xFF444444);

    // 3. Header
    const std::string title = isEs ? "ADMINISTRADOR DE WAYPOINTS" : "WAYPOINT MANAGER";
    drawCenteredString(fontRenderer, title, panelX + panelW / 2, panelY + 6, 0xFFFFFF);

    int_t headerOffset = 18;
    if (isSplit)
    {
        std::string playerTag;
        int_t tagColor = 0xFFFFFF;
        if (m_playerIndex == 1)
        {
            playerTag = isEs ? "[ JUGADOR 2 - MANDO 2 ]" : "[ PLAYER 2 - CONTROLLER 2 ]";
            tagColor = 0xFFAAF5;
        }
        else
        {
            playerTag = isEs ? "[ JUGADOR 1 - MANDO 1 ]" : "[ PLAYER 1 - CONTROLLER 1 ]";
            tagColor = 0x55FFFF;
        }
        drawCenteredString(fontRenderer, playerTag, panelX + panelW / 2, panelY + headerOffset, tagColor);
        headerOffset += 11;
    }

    // Divider line
    drawRect(panelX + 4, panelY + headerOffset, panelX + panelW - 4, panelY + headerOffset + 1, 0xFF333333);

    // 4. Content Area
    const int_t listY = panelY + headerOffset + 4;
    const int_t rowHeight = 22;
    const int_t availableH = panelY + panelH - listY - 4;
    m_maxVisibleRows = std::max<int_t>(1, availableH / rowHeight);

    const size_t totalWaypoints = ReiMinimap::getInstance().getWaypointCount();

    if (totalWaypoints == 0)
    {
        const std::string emptyMsg = isEs ? "No hay waypoints guardados" : "No waypoints saved";
        const std::string hintMsg = isEs ? "Usa Triangulo + Arriba para agregar uno." : "Press Triangle + Up to add one.";
        drawCenteredString(fontRenderer, emptyMsg, panelX + panelW / 2, listY + 30, 0x888888);
        drawCenteredString(fontRenderer, hintMsg, panelX + panelW / 2, listY + 46, 0x555555);
    }
    else
    {
        // Clamp selection
        if (m_selectedIndex < 0)
            m_selectedIndex = 0;
        if (m_selectedIndex >= static_cast<int_t>(totalWaypoints))
            m_selectedIndex = static_cast<int_t>(totalWaypoints) - 1;

        // Auto-scroll
        if (m_selectedIndex < m_scrollOffset)
            m_scrollOffset = m_selectedIndex;
        if (m_selectedIndex >= m_scrollOffset + m_maxVisibleRows)
            m_scrollOffset = m_selectedIndex - m_maxVisibleRows + 1;

        const int_t maxScroll = std::max<int_t>(0, static_cast<int_t>(totalWaypoints) - m_maxVisibleRows);
        m_scrollOffset = std::clamp(m_scrollOffset, 0, maxScroll);

        // Counter in top right of panel
        char counterBuf[32];
        std::snprintf(counterBuf, sizeof(counterBuf), "(%d/%u)", m_selectedIndex + 1, static_cast<unsigned>(totalWaypoints));
        drawString(fontRenderer, counterBuf, panelX + panelW - fontRenderer->getStringWidth(counterBuf) - 8, panelY + 6, 0xAAAAAA);

        const int_t rowsToDraw = std::min<int_t>(m_maxVisibleRows, static_cast<int_t>(totalWaypoints) - m_scrollOffset);
        for (int_t r = 0; r < rowsToDraw; ++r)
        {
            const int_t itemIdx = m_scrollOffset + r;
            const Waypoint &wp = ReiMinimap::getInstance().getWaypoint(static_cast<size_t>(itemIdx));
            const int_t itemY = listY + r * rowHeight;
            const bool isSelected = (itemIdx == m_selectedIndex);

            // Selection background
            if (isSelected)
            {
                drawRect(panelX + 4, itemY, panelX + panelW - 14, itemY + rowHeight - 2, 0x30FFFFFF);
                drawRect(panelX + 4, itemY, panelX + 7, itemY + rowHeight - 2, 0xFFFFAA00);
            }

            // Swatch indicator square (8x8)
            const int_t swatchX = panelX + 11;
            const int_t swatchY = itemY + 5;
            drawRect(swatchX - 1, swatchY - 1, swatchX + 9, swatchY + 9, 0xFF000000);
            if (wp.enabled)
            {
                drawRect(swatchX, swatchY, swatchX + 8, swatchY + 8, 0xFF000000 | wp.color);
            }
            else
            {
                drawRect(swatchX, swatchY, swatchX + 8, swatchY + 8, 0xFF444444);
                // Draw small dark diagonal line through disabled swatch
                drawRect(swatchX + 1, swatchY + 1, swatchX + 7, swatchY + 3, 0xFF222222);
            }

            // Waypoint title & order number
            std::string label = std::to_string(itemIdx + 1) + ". " + wp.name;
            if (!wp.enabled)
                label += isEs ? " [Oculto]" : " [Hidden]";

            const int_t nameColor = wp.enabled ? (isSelected ? 0xFFFFAA : 0xFFFFFF) : 0x888888;
            drawString(fontRenderer, label, panelX + 24, itemY + 2, nameColor);

            // Coordinates
            char coordBuf[48];
            std::snprintf(coordBuf, sizeof(coordBuf), "X:%d  Y:%d  Z:%d", wp.x, wp.y, wp.z);
            const int_t coordColor = wp.enabled ? (isSelected ? 0xDDDDDD : 0x888888) : 0x555555;
            drawString(fontRenderer, coordBuf, panelX + 24, itemY + 11, coordColor);
        }

        // Scrollbar on right edge
        if (totalWaypoints > static_cast<size_t>(m_maxVisibleRows))
        {
            const int_t trackX = panelX + panelW - 9;
            const int_t trackY = listY;
            const int_t trackH = m_maxVisibleRows * rowHeight - 2;

            drawRect(trackX, trackY, trackX + 3, trackY + trackH, 0x60000000);

            const int_t thumbH = std::max<int_t>(8, (trackH * m_maxVisibleRows) / static_cast<int_t>(totalWaypoints));
            const int_t thumbY = trackY + ((trackH - thumbH) * m_scrollOffset) / maxScroll;
            drawRect(trackX, thumbY, trackX + 3, thumbY + thumbH, 0xFFAAAAAA);

            // Scroll indicator arrows
            if (m_scrollOffset > 0)
                drawString(fontRenderer, "^", trackX - 1, trackY - 8, 0xFFFFAA);
            if (m_scrollOffset + m_maxVisibleRows < static_cast<int_t>(totalWaypoints))
                drawString(fontRenderer, "v", trackX - 1, trackY + trackH + 1, 0xFFFFAA);
        }
    }

    // 5. Bottom Legacy Control Hints Row
#if PLATFORM_PS2
    const std::string buttons[] = {"D-Pad", "Cross", "Square", "Circle"};
    const std::string actions[] = {
        isEs ? "Navegar" : "Navigate",
        isEs ? "Alternar" : "Toggle",
        isEs ? "Eliminar" : "Delete",
        isEs ? "Volver" : "Back"
    };
    drawControlHintRow(mc, width, legacyHintRowY(height, isSplit), buttons, actions, 4);
#elif PLATFORM_XBOX
    const std::string buttons[] = {"D-Pad", "A", "X", "B"};
    const std::string actions[] = {
        isEs ? "Navegar" : "Navigate",
        isEs ? "Alternar" : "Toggle",
        isEs ? "Eliminar" : "Delete",
        isEs ? "Volver" : "Back"
    };
    drawControlHintRow(mc, width, legacyHintRowY(height, isSplit), buttons, actions, 4);
#else
    const std::string buttons[] = {"Up/Down", "Space/Enter", "Delete", "Esc"};
    const std::string actions[] = {
        isEs ? "Navegar" : "Navigate",
        isEs ? "Alternar" : "Toggle",
        isEs ? "Eliminar" : "Delete",
        isEs ? "Volver" : "Back"
    };
    drawControlHintRow(mc, width, legacyHintRowY(height, isSplit), buttons, actions, 4);
#endif

    GuiScreen::drawScreen(mouseX, mouseY, partialTick);
}
