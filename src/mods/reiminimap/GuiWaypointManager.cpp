#include "GuiWaypointManager.h"
#include "GuiMinimapMenu.h"
#include "ReiMinimap.h"
#include "Waypoint.h"

#include "net/minecraft/src/Minecraft.h"
#include "net/minecraft/src/GameSettings.h"
#include "net/minecraft/src/EntityPlayerSP.h"
#include "net/minecraft/src/FontRenderer.h"
#include "net/minecraft/src/GuiTextField.h"
#include "net/minecraft/src/SoundManager.h"
#include "net/minecraft/src/StringTranslate.h"
#include "net/minecraft/src/UiStrings.h"
#include "net/minecraft/src/legacy/LegacyMenuHints.h"
#include "net/minecraft/src/legacy/LegacyMenuWidgets.h"
#include "net/minecraft/src/legacy/LegacyOptionText.h"
#include "net/minecraft/src/legacy/LegacyOptionsLayout.h"
#include "net/minecraft/src/legacy/LegacyOptionsPanel.h"
#include "net/minecraft/src/ControlIcon.h"
#include "platform/PlatformConfig.h"
#include "platform/RenderAPI.h"
#include "platform/Input.h"

#if defined(PS2_PLATFORM) || defined(WII_PLATFORM) || defined(XBOX_PLATFORM)
#include "net/minecraft/src/VirtualKeyboard.h"
#endif

#if PLATFORM_PS2
#include "ps2/input/Ps2PadState.h"
#endif

#if !PLATFORM_PS2 && !PLATFORM_WII && !PLATFORM_XBOX
#include "pc/lwjgl/Keyboard.h"
#endif

#include <algorithm>
#include <cmath>
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
    , m_editMode(EditMode::None)
    , m_nameField(nullptr)
    , m_triangleLatch(true)
    , m_downLatch(true)
    , m_actionLatch(true)
    , m_squareLatch(true)
    , m_circleLatch(true)
    , m_openLatch(true)
    , m_prevHeld(0)
    , m_prevLeftY(128)
{
}

GuiWaypointManager::~GuiWaypointManager()
{
    delete m_nameField;
    m_nameField = nullptr;
}

bool GuiWaypointManager::isLegacyMode() const
{
    return mc != nullptr && mc->gameSettings != nullptr && mc->gameSettings->legacyUI;
}

bool GuiWaypointManager::usesSpecializedMenuNavigation() const
{
    return m_editMode == EditMode::None;
}

void GuiWaypointManager::initGui()
{
    m_scrollOffset = 0;
    m_editMode = EditMode::None;
    m_prevHeld = 0;
    m_prevLeftY = 128;
    m_triangleLatch = true;
    m_downLatch = true;
    m_actionLatch = true;
    m_squareLatch = true;
    m_circleLatch = true;
    m_openLatch = true;

    delete m_nameField;
    const int_t fieldW = std::min<int_t>(200, width - 40);
    m_nameField = new GuiTextField(this, fontRenderer, (width - fieldW) / 2, height / 2 - 44, fieldW, 18, "");
    m_nameField->setMaxStringLength(24);
    m_nameField->setFocused(false);
}

void GuiWaypointManager::onGuiClosed()
{
#if defined(PS2_PLATFORM) || defined(WII_PLATFORM) || defined(XBOX_PLATFORM)
    VirtualKeyboard::instance().releaseFocus();
#endif
    if (m_nameField != nullptr)
        m_nameField->setFocused(false);
    GuiScreen::onGuiClosed();
}

void GuiWaypointManager::beginCreateWaypoint()
{
    if (m_nameField == nullptr)
        return;
    m_editMode = EditMode::CreateNew;
    char buf[32];
    std::snprintf(buf, sizeof(buf), "Waypoint %u", static_cast<unsigned>(ReiMinimap::getInstance().getWaypointCount() + 1));
    m_nameField->setText(buf);
    m_nameField->setFocused(true);
    if (mc != nullptr && mc->sndManager != nullptr)
        mc->sndManager->playSoundFX("random.click", 1.0f, 1.0f);
}

void GuiWaypointManager::beginRenameWaypoint()
{
    const size_t total = ReiMinimap::getInstance().getWaypointCount();
    if (m_nameField == nullptr || total == 0 || m_selectedIndex < 0 || m_selectedIndex >= static_cast<int>(total))
        return;
    m_editMode = EditMode::RenameExisting;
    m_nameField->setText(ReiMinimap::getInstance().getWaypoint(static_cast<size_t>(m_selectedIndex)).name);
    m_nameField->setFocused(true);
    if (mc != nullptr && mc->sndManager != nullptr)
        mc->sndManager->playSoundFX("random.click", 1.0f, 1.0f);
}

void GuiWaypointManager::commitTextEdit()
{
    if (m_editMode == EditMode::None || m_nameField == nullptr)
        return;

    std::string text = m_nameField->getText();
    if (m_editMode == EditMode::CreateNew)
    {
        EntityPlayer *p = nullptr;
        if (mc != nullptr)
        {
            if (m_playerIndex == 1 && mc->thePlayer2 != nullptr)
                p = mc->thePlayer2;
            else
                p = mc->thePlayerOne ? mc->thePlayerOne : mc->thePlayer;
        }
        int_t px = p ? static_cast<int_t>(std::floor(p->posX)) : 0;
        int_t py = p ? static_cast<int_t>(std::floor(p->posY)) : 64;
        int_t pz = p ? static_cast<int_t>(std::floor(p->posZ)) : 0;
        if (text.empty())
            text = "Waypoint";
        ReiMinimap::getInstance().addWaypoint(text, px, py, pz);
        m_selectedIndex = static_cast<int>(ReiMinimap::getInstance().getWaypointCount()) - 1;
        if (mc != nullptr && mc->sndManager != nullptr)
            mc->sndManager->playSoundFX("random.orb", 1.0f, 1.0f);
    }
    else if (m_editMode == EditMode::RenameExisting)
    {
        if (!text.empty() && m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(ReiMinimap::getInstance().getWaypointCount()))
        {
            ReiMinimap::getInstance().renameWaypoint(static_cast<size_t>(m_selectedIndex), text);
            if (mc != nullptr && mc->sndManager != nullptr)
                mc->sndManager->playSoundFX("random.click", 1.0f, 1.1f);
        }
    }
    cancelTextEdit();
}

void GuiWaypointManager::cancelTextEdit()
{
    m_editMode = EditMode::None;
    m_openLatch = true;
    if (m_nameField != nullptr)
        m_nameField->setFocused(false);
#if defined(PS2_PLATFORM) || defined(WII_PLATFORM) || defined(XBOX_PLATFORM)
    VirtualKeyboard::instance().releaseFocus();
#endif
}

void GuiWaypointManager::updateScreen()
{
    GuiScreen::updateScreen();
    if (m_editMode != EditMode::None && m_nameField != nullptr)
        m_nameField->updateCursorCounter();
}

void GuiWaypointManager::handleSpecializedMenuInput()
{
    if (m_editMode != EditMode::None)
        return;

#if PLATFORM_PS2
    const Ps2PadSnapshot &pad = ps2PadGetSnapshot(m_playerIndex);
    if (!pad.connected)
        return;

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

    if (!m_circleLatch && (pressed & PS2_PAD_CIRCLE))
    {
        closeScreen();
        return;
    }

    const size_t totalWaypoints = ReiMinimap::getInstance().getWaypointCount();

    if ((pressed & PS2_PAD_UP) || (pad.leftY < 64 && m_prevLeftY >= 64))
    {
        if (totalWaypoints > 0)
        {
            m_selectedIndex = (m_selectedIndex > 0) ? (m_selectedIndex - 1) : (static_cast<int>(totalWaypoints) - 1);
            if (mc != nullptr && mc->sndManager != nullptr)
                mc->sndManager->playSoundFX("random.focus", 1.0f, 1.0f);
        }
    }
    else if ((!m_downLatch && (pressed & PS2_PAD_DOWN)) || (pad.leftY > 192 && m_prevLeftY <= 192))
    {
        if (totalWaypoints > 0)
        {
            m_selectedIndex = (m_selectedIndex + 1 < static_cast<int>(totalWaypoints)) ? (m_selectedIndex + 1) : 0;
            if (mc != nullptr && mc->sndManager != nullptr)
                mc->sndManager->playSoundFX("random.focus", 1.0f, 1.0f);
        }
    }
    m_prevLeftY = pad.leftY;

    if (!m_actionLatch && (pressed & PS2_PAD_CROSS))
    {
        if (totalWaypoints > 0 && m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(totalWaypoints))
        {
            ReiMinimap::getInstance().toggleWaypoint(static_cast<size_t>(m_selectedIndex));
            if (mc != nullptr && mc->sndManager != nullptr)
                mc->sndManager->playSoundFX("random.click", 1.0f, 1.0f);
        }
    }
    if (!m_triangleLatch && (pressed & PS2_PAD_TRIANGLE))
    {
        beginCreateWaypoint();
        return;
    }
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

    if (m_openLatch)
    {
        if ((pad.held & (PLATFORM_TEXT_TYPE | PLATFORM_TEXT_CLOSE | PLATFORM_TEXT_BACK | PLATFORM_TEXT_SHIFT |
                         PLATFORM_TEXT_TAB_LEFT | PLATFORM_TEXT_TAB_RIGHT)) == 0)
            m_openLatch = false;
        return;
    }

    if (pad.pressed & PLATFORM_TEXT_CLOSE)
    {
        closeScreen();
        return;
    }

    const size_t totalWaypoints = ReiMinimap::getInstance().getWaypointCount();

    if (pad.pressed & PLATFORM_TEXT_UP)
    {
        if (totalWaypoints > 0)
        {
            m_selectedIndex = (m_selectedIndex > 0) ? (m_selectedIndex - 1) : (static_cast<int>(totalWaypoints) - 1);
            if (mc != nullptr && mc->sndManager != nullptr)
                mc->sndManager->playSoundFX("random.focus", 1.0f, 1.0f);
        }
    }
    else if (pad.pressed & PLATFORM_TEXT_DOWN)
    {
        if (totalWaypoints > 0)
        {
            m_selectedIndex = (m_selectedIndex + 1 < static_cast<int>(totalWaypoints)) ? (m_selectedIndex + 1) : 0;
            if (mc != nullptr && mc->sndManager != nullptr)
                mc->sndManager->playSoundFX("random.focus", 1.0f, 1.0f);
        }
    }
    else if (pad.pressed & PLATFORM_TEXT_LEFT)
    {
        if (totalWaypoints > 0 && m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(totalWaypoints))
        {
            ReiMinimap::getInstance().cycleWaypointColor(static_cast<size_t>(m_selectedIndex), -1);
            if (mc != nullptr && mc->sndManager != nullptr)
                mc->sndManager->playSoundFX("random.click", 1.0f, 0.9f);
        }
    }
    else if (pad.pressed & PLATFORM_TEXT_RIGHT)
    {
        if (totalWaypoints > 0 && m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(totalWaypoints))
        {
            ReiMinimap::getInstance().cycleWaypointColor(static_cast<size_t>(m_selectedIndex), 1);
            if (mc != nullptr && mc->sndManager != nullptr)
                mc->sndManager->playSoundFX("random.click", 1.0f, 1.1f);
        }
    }

    // A: Toggle visibility
    if (pad.pressed & PLATFORM_TEXT_TYPE)
    {
        if (totalWaypoints > 0 && m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(totalWaypoints))
        {
            ReiMinimap::getInstance().toggleWaypoint(static_cast<size_t>(m_selectedIndex));
            if (mc != nullptr && mc->sndManager != nullptr)
                mc->sndManager->playSoundFX("random.click", 1.0f, 1.0f);
        }
    }

    // Y (SHIFT): Create new waypoint with VirtualKeyboard
    if (pad.pressed & PLATFORM_TEXT_SHIFT)
    {
        beginCreateWaypoint();
        return;
    }

    // White (TAB_LEFT): Rename selected waypoint with VirtualKeyboard
    if (pad.pressed & PLATFORM_TEXT_TAB_LEFT)
    {
        beginRenameWaypoint();
        return;
    }

    // Black (TAB_RIGHT): Open Minimap Options
    if (pad.pressed & PLATFORM_TEXT_TAB_RIGHT)
    {
        if (mc != nullptr)
        {
            if (mc->sndManager != nullptr)
                mc->sndManager->playSoundFX("random.click", 1.0f, 1.0f);
            if (mc->isSplitScreenActive() && m_playerIndex == 1)
                mc->displayPlayerScreen(1, new GuiMinimapMenu(nullptr, 1));
            else
                mc->displayGuiScreen(new GuiMinimapMenu(this, m_playerIndex));
        }
        return;
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
    if (m_editMode != EditMode::None && m_nameField != nullptr)
    {
        if (key == 1) // ESC / B cancel
        {
            cancelTextEdit();
            return;
        }
        if (key == 28 || c == '\r' || c == '\n') // Enter / Start confirm
        {
            commitTextEdit();
            return;
        }
        m_nameField->textboxKeyTyped(c, key);
        return;
    }

#if !PLATFORM_PS2 && !PLATFORM_WII && !PLATFORM_XBOX
    const size_t totalWaypoints = ReiMinimap::getInstance().getWaypointCount();
    if (key == lwjgl::Keyboard::KEY_UP)
    {
        if (totalWaypoints > 0)
            m_selectedIndex = (m_selectedIndex > 0) ? (m_selectedIndex - 1) : (static_cast<int>(totalWaypoints) - 1);
        return;
    }
    if (key == lwjgl::Keyboard::KEY_DOWN)
    {
        if (totalWaypoints > 0)
            m_selectedIndex = (m_selectedIndex + 1 < static_cast<int>(totalWaypoints)) ? (m_selectedIndex + 1) : 0;
        return;
    }
    if (key == lwjgl::Keyboard::KEY_LEFT && totalWaypoints > 0)
    {
        ReiMinimap::getInstance().cycleWaypointColor(static_cast<size_t>(m_selectedIndex), -1);
        return;
    }
    if (key == lwjgl::Keyboard::KEY_RIGHT && totalWaypoints > 0)
    {
        ReiMinimap::getInstance().cycleWaypointColor(static_cast<size_t>(m_selectedIndex), 1);
        return;
    }
    if (key == lwjgl::Keyboard::KEY_N)
    {
        beginCreateWaypoint();
        return;
    }
    if (key == lwjgl::Keyboard::KEY_R)
    {
        beginRenameWaypoint();
        return;
    }
    if (key == lwjgl::Keyboard::KEY_SPACE || key == lwjgl::Keyboard::KEY_RETURN)
    {
        if (totalWaypoints > 0 && m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(totalWaypoints))
            ReiMinimap::getInstance().toggleWaypoint(static_cast<size_t>(m_selectedIndex));
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
    cancelTextEdit();
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

// LCE look: grey bevelled panel, one stone button per waypoint (colour swatch,
// name, coordinates, TickBox = shown on the map), yellow label on the selected one.
void GuiWaypointManager::drawLegacyScreen(float_t partialTick)
{
    const bool isEs = isSpanishLocale();
    const bool isSplit = (mc != nullptr && mc->isSplitScreenActive());
    legacyDrawMenuBackdrop(mc, width, height, partialTick);

    const size_t totalWaypoints = ReiMinimap::getInstance().getWaypointCount();
    const int_t total = static_cast<int_t>(totalWaypoints);
    const bool compact = (height < 180);
    const int_t rowH = compact ? 20 : (height >= 330 ? 24 : 22);
    const int_t headerH = (isSplit && !compact) ? 30 : (compact ? 16 : 20);
    const LegacyMenuLayout l = legacyMenuLayout(mc, width, height, 280,
        std::max<int_t>(total, 3), headerH, compact ? 0 : 14, rowH);
    legacyDrawMenuPanel(l.panelX, l.panelY, l.panelW, l.panelH);
    m_maxVisibleRows = l.visibleRows;

    const std::string title = isEs ? "Waypoints del minimapa" : "Minimap Waypoints";
    legacyDrawPanelTitle(fontRenderer, title, l.panelX + l.panelW / 2, l.titleY);
    if (isSplit && !compact)
    {
        const std::string playerTag = (m_playerIndex == 1)
            ? (isEs ? "Jugador 2" : "Player 2")
            : (isEs ? "Jugador 1" : "Player 1");
        legacyDrawPanelText(fontRenderer, playerTag,
            l.panelX + (l.panelW - fontRenderer->getStringWidth(playerTag)) / 2, l.titleY + 11,
            static_cast<int_t>(0xff606060u));
    }

    if (total == 0)
    {
        const std::string a = isEs ? "No hay waypoints guardados" : "No waypoints saved";
        const std::string b = isEs ? "Pulsa Y para crear uno nuevo" : "Press Y to create a new one";
        const int_t midY = l.firstRowY + (l.visibleRows * (l.rowH + l.rowGap)) / 2 - 10;
        legacyDrawPanelText(fontRenderer, a, l.panelX + (l.panelW - fontRenderer->getStringWidth(a)) / 2, midY);
        legacyDrawPanelText(fontRenderer, b, l.panelX + (l.panelW - fontRenderer->getStringWidth(b)) / 2, midY + 12,
            static_cast<int_t>(0xff606060u));
    }
    else
    {
        if (m_selectedIndex < 0) m_selectedIndex = 0;
        if (m_selectedIndex >= total) m_selectedIndex = total - 1;
        m_scrollOffset = legacyMenuKeepVisible(m_selectedIndex, total, l.visibleRows, m_scrollOffset);

        char counterBuf[32];
        std::snprintf(counterBuf, sizeof(counterBuf), "%d/%u", m_selectedIndex + 1, static_cast<unsigned>(totalWaypoints));
        legacyDrawPanelText(fontRenderer, counterBuf,
            l.panelX + l.panelW - fontRenderer->getStringWidth(counterBuf) - 8, l.titleY, static_cast<int_t>(0xff606060u));

        const int_t rowW = l.contentW - 8;
        for (int_t vi = 0; vi < l.visibleRows; ++vi)
        {
            const int_t itemIdx = m_scrollOffset + vi;
            if (itemIdx >= total)
                break;
            const Waypoint &wp = ReiMinimap::getInstance().getWaypoint(static_cast<size_t>(itemIdx));
            const bool sel = (itemIdx == m_selectedIndex);
            const int_t ry = l.rowY(vi);
            const int_t labelColor = legacyDrawMenuButtonFrame(mc, l.contentX, ry, rowW, l.rowH, sel, true);

            // Colour swatch (grey when hidden)
            const int_t sw = 8;
            const int_t sx = l.contentX + 6;
            const int_t sy = ry + (l.rowH - sw) / 2;
            drawRect(sx - 1, sy - 1, sx + sw + 1, sy + sw + 1, static_cast<int_t>(0xFF000000u));
            drawRect(sx, sy, sx + sw, sy + sw,
                wp.enabled ? (static_cast<int_t>(0xFF000000u) | wp.color) : static_cast<int_t>(0xFF555555u));

            // "Shown on the map" tick on the right
            const int_t tickX = l.contentX + rowW - 17;
            legacyDrawTickBox(mc, tickX, ry + (l.rowH - 12) / 2, wp.enabled, sel);

            const int_t textX = sx + sw + 6;
            const int_t textW = tickX - 4 - textX;
            const std::string label = std::to_string(itemIdx + 1) + ". " + wp.name;
            char coordBuf[64];
            std::snprintf(coordBuf, sizeof(coordBuf), "X:%d Y:%d Z:%d", wp.x, wp.y, wp.z);
            if (l.rowH >= 20)
            {
                const int_t top = ry + (l.rowH - 18) / 2;
                legacyDrawOptionText(fontRenderer, legacyFitText(fontRenderer, label, textW), textX, top,
                    wp.enabled ? labelColor : static_cast<int_t>(0xffc0c0c0u));
                legacyDrawOptionText(fontRenderer, legacyFitText(fontRenderer, coordBuf, textW), textX, top + 10,
                    sel ? static_cast<int_t>(0xffe0e0e0u) : static_cast<int_t>(0xffc8c8c8u));
            }
            else
            {
                legacyDrawOptionText(fontRenderer, legacyFitText(fontRenderer, label, textW), textX,
                    ry + (l.rowH - 7) / 2, wp.enabled ? labelColor : static_cast<int_t>(0xffc0c0c0u));
            }
        }
        const int_t listH = l.visibleRows * l.rowH + (l.visibleRows - 1) * l.rowGap;
        legacyDrawMenuScrollbar(l.panelX + l.panelW - 10, l.firstRowY, listH, m_scrollOffset, l.visibleRows, total);
    }

    const bool editing = (m_editMode != EditMode::None && m_nameField != nullptr);

#if PLATFORM_XBOX
    // The bottom row holds four prompts; the rest go on the panel.
    if (!compact && !editing)
    {
        const std::string ib[] = {"White", "Black", "D-Pad"};
        const std::string ia[] = {
            isEs ? "Nombre" : "Rename",
            isEs ? "Opciones" : "Options",
            isEs ? "Color" : "Colour"
        };
        legacyDrawInlineHints(mc, l.panelX + l.panelW / 2, l.footerY + 3, ib, ia, 3);
    }
#endif

    if (editing)
    {
        // The text field sits at height / 2 - 44 (initGui); the small panel wraps it.
        const int_t boxW = std::min<int_t>(232, width - 8);
        const int_t boxX = (width - boxW) / 2;
        const int_t boxY = std::max<int_t>(4, height / 2 - 70);
        legacyDrawMenuPanel(boxX, boxY, boxW, 54);
        const std::string editTitle = legacyFitText(fontRenderer, (m_editMode == EditMode::CreateNew)
            ? (isEs ? "Nuevo waypoint (Start: guardar)" : "New waypoint (Start: save)")
            : (isEs ? "Renombrar waypoint (Start: guardar)" : "Rename waypoint (Start: save)"), boxW - 12);
        legacyDrawPanelTitle(fontRenderer, editTitle, width / 2, boxY + 8);
        m_nameField->drawTextBox();
        return;
    }

#if PLATFORM_XBOX
    const std::string buttons[] = {"A", "Y", "X", "B"};
    const std::string actions[] = {
        isEs ? "Ver" : "Toggle",
        isEs ? "Nuevo" : "New",
        isEs ? "Borrar" : "Delete",
        isEs ? "Salir" : "Back"
    };
    drawControlHintRow(mc, width, l.hintY, buttons, actions, 4);
#elif PLATFORM_PS2
    const std::string buttons[] = {"Cross", "Triangle", "Square", "Circle"};
    const std::string actions[] = {
        isEs ? "Alternar" : "Toggle",
        isEs ? "Nuevo" : "New",
        isEs ? "Eliminar" : "Delete",
        isEs ? "Volver" : "Back"
    };
    drawControlHintRow(mc, width, l.hintY, buttons, actions, 4);
#else
    const std::string buttons[] = {"Space", "N/R", "Del", "Esc"};
    const std::string actions[] = {
        isEs ? "Alternar" : "Toggle",
        isEs ? "Nuevo/Ren" : "New/Ren",
        isEs ? "Eliminar" : "Delete",
        isEs ? "Volver" : "Back"
    };
    drawControlHintRow(mc, width, l.hintY, buttons, actions, 4);
#endif
}

void GuiWaypointManager::drawScreen(int_t mouseX, int_t mouseY, float_t partialTick)
{
    if (isLegacyMode())
    {
        drawLegacyScreen(partialTick);
        GuiScreen::drawScreen(mouseX, mouseY, partialTick);
        return;
    }

    const bool isEs = isSpanishLocale();
    const bool isSplit = (mc != nullptr && mc->isSplitScreenActive());
    const bool legacy = isLegacyMode();

    // 1. Background overlay
    drawGradientRect(0, 0, width, height, static_cast<int_t>(0xC0050505u), static_cast<int_t>(0xE0101010u));

    // 2. Central Card (Legacy framed panel in Legacy UI, dark slate card in Java UI)
    const int_t hintY = legacyHintRowY(height, isSplit);
    const int_t panelW = std::min<int_t>(292, width - 16);
    const int_t panelH = std::min<int_t>(184, hintY - 14);
    const int_t panelX = (width - panelW) / 2;
    const int_t panelY = std::max<int_t>(8, (hintY - panelH) / 2);

    if (legacy)
    {
        LegacyOptionsLayout layout{};
        layout.panelX = panelX;
        layout.panelY = panelY;
        layout.panelWidth = panelW;
        layout.panelHeight = panelH;
        LegacyOptionsPanel panelRenderer;
        panelRenderer.draw(layout);
    }
    else
    {
        drawRect(panelX, panelY, panelX + panelW, panelY + panelH, static_cast<int_t>(0xD8121212u));
        drawRect(panelX, panelY, panelX + panelW, panelY + 1, static_cast<int_t>(0xFF555555u));
        drawRect(panelX, panelY + panelH - 1, panelX + panelW, panelY + panelH, static_cast<int_t>(0xFF555555u));
        drawRect(panelX, panelY, panelX + 1, panelY + panelH, static_cast<int_t>(0xFF555555u));
        drawRect(panelX + panelW - 1, panelY, panelX + panelW, panelY + panelH, static_cast<int_t>(0xFF555555u));
    }

    // 3. Header
    const std::string title = isEs ? "WAYPOINTS DEL MINIMAPA" : "MINIMAP WAYPOINTS";
    drawCenteredString(fontRenderer, title, panelX + panelW / 2, panelY + 6, 0xFFFFFF);

    int_t headerOffset = 18;
    if (isSplit)
    {
        std::string playerTag = (m_playerIndex == 1)
            ? (isEs ? "[ JUGADOR 2 ]" : "[ PLAYER 2 ]")
            : (isEs ? "[ JUGADOR 1 ]" : "[ PLAYER 1 ]");
        drawCenteredString(fontRenderer, playerTag, panelX + panelW / 2, panelY + headerOffset,
            (m_playerIndex == 1) ? 0xFFAAF5 : 0x55FFFF);
        headerOffset += 10;
    }

    drawRect(panelX + 6, panelY + headerOffset, panelX + panelW - 6, panelY + headerOffset + 1, static_cast<int_t>(0xFF444444u));

    // 4. Waypoints List
    const int_t listY = panelY + headerOffset + 4;
    const int_t rowHeight = 22;
    const int_t availableH = panelY + panelH - listY - 6;
    m_maxVisibleRows = std::max<int_t>(1, availableH / rowHeight);

    const size_t totalWaypoints = ReiMinimap::getInstance().getWaypointCount();

    if (totalWaypoints == 0)
    {
        drawCenteredString(fontRenderer, isEs ? "No hay waypoints guardados" : "No waypoints saved",
            panelX + panelW / 2, listY + 24, 0xAAAAAA);
        drawCenteredString(fontRenderer, isEs ? "Pulsa Y para crear uno nuevo" : "Press Y to create a new one",
            panelX + panelW / 2, listY + 38, 0x777777);
    }
    else
    {
        if (m_selectedIndex < 0) m_selectedIndex = 0;
        if (m_selectedIndex >= static_cast<int_t>(totalWaypoints))
            m_selectedIndex = static_cast<int_t>(totalWaypoints) - 1;

        if (m_selectedIndex < m_scrollOffset) m_scrollOffset = m_selectedIndex;
        if (m_selectedIndex >= m_scrollOffset + m_maxVisibleRows)
            m_scrollOffset = m_selectedIndex - m_maxVisibleRows + 1;

        const int_t maxScroll = std::max<int_t>(0, static_cast<int_t>(totalWaypoints) - m_maxVisibleRows);
        m_scrollOffset = std::clamp(m_scrollOffset, 0, maxScroll);

        char counterBuf[32];
        std::snprintf(counterBuf, sizeof(counterBuf), "(%d/%u)", m_selectedIndex + 1, static_cast<unsigned>(totalWaypoints));
        drawString(fontRenderer, counterBuf, panelX + panelW - fontRenderer->getStringWidth(counterBuf) - 8, panelY + 6, 0xCCCCCC);

        const int_t rowsToDraw = std::min<int_t>(m_maxVisibleRows, static_cast<int_t>(totalWaypoints) - m_scrollOffset);
        for (int_t r = 0; r < rowsToDraw; ++r)
        {
            const int_t itemIdx = m_scrollOffset + r;
            const Waypoint &wp = ReiMinimap::getInstance().getWaypoint(static_cast<size_t>(itemIdx));
            const int_t itemY = listY + r * rowHeight;
            const bool isSelected = (itemIdx == m_selectedIndex);

            if (isSelected)
            {
                drawRect(panelX + 6, itemY, panelX + panelW - 14, itemY + rowHeight - 2,
                    legacy ? static_cast<int_t>(0xD03A5288u) : static_cast<int_t>(0x38FFFFFFu));
                drawRect(panelX + 6, itemY, panelX + 9, itemY + rowHeight - 2, static_cast<int_t>(0xFFFFDD44u));
            }

            const int_t swatchX = panelX + 13;
            const int_t swatchY = itemY + 5;
            drawRect(swatchX - 1, swatchY - 1, swatchX + 9, swatchY + 9, static_cast<int_t>(0xFF000000u));
            if (wp.enabled)
                drawRect(swatchX, swatchY, swatchX + 8, swatchY + 8, static_cast<int_t>(0xFF000000u) | wp.color);
            else
                drawRect(swatchX, swatchY, swatchX + 8, swatchY + 8, static_cast<int_t>(0xFF444444u));

            std::string label = std::to_string(itemIdx + 1) + ". " + wp.name;
            if (!wp.enabled)
                label += isEs ? " [Oculto]" : " [Hidden]";
            drawString(fontRenderer, label, panelX + 26, itemY + 2,
                wp.enabled ? (isSelected ? 0xFFFFAA : 0xFFFFFF) : 0x888888);

            char coordBuf[64];
            std::snprintf(coordBuf, sizeof(coordBuf), "X:%d Y:%d Z:%d%s",
                wp.x, wp.y, wp.z, isSelected ? (isEs ? "  <Color>" : "  <Color>") : "");
            drawString(fontRenderer, coordBuf, panelX + 26, itemY + 11,
                wp.enabled ? (isSelected ? 0xDDDDDD : 0x999999) : 0x666666);
        }

        if (totalWaypoints > static_cast<size_t>(m_maxVisibleRows))
        {
            const int_t trackX = panelX + panelW - 10;
            const int_t trackY = listY;
            const int_t trackH = m_maxVisibleRows * rowHeight - 2;
            drawRect(trackX, trackY, trackX + 3, trackY + trackH, static_cast<int_t>(0x60000000u));
            const int_t thumbH = std::max<int_t>(8, (trackH * m_maxVisibleRows) / static_cast<int_t>(totalWaypoints));
            const int_t thumbY = trackY + ((trackH - thumbH) * m_scrollOffset) / maxScroll;
            drawRect(trackX, thumbY, trackX + 3, thumbY + thumbH, static_cast<int_t>(0xFFCCCCCCu));
        }
    }

    // 5. Inline Name Editor modal (paired with Xbox VirtualKeyboard)
    if (m_editMode != EditMode::None && m_nameField != nullptr)
    {
        const int_t boxW = std::min<int_t>(224, width - 20);
        const int_t boxH = 54;
        const int_t boxX = (width - boxW) / 2;
        const int_t boxY = std::max<int_t>(12, height / 2 - 70);
        drawRect(boxX - 2, boxY - 2, boxX + boxW + 2, boxY + boxH + 2, static_cast<int_t>(0xFFAAAAAAu));
        drawRect(boxX, boxY, boxX + boxW, boxY + boxH, static_cast<int_t>(0xF0181818u));

        const std::string editTitle = (m_editMode == EditMode::CreateNew)
            ? (isEs ? "Nuevo Waypoint (Start: Guardar)" : "New Waypoint (Start: Save)")
            : (isEs ? "Renombrar Waypoint (Start: Guardar)" : "Rename Waypoint (Start: Save)");
        drawCenteredString(fontRenderer, editTitle, width / 2, boxY + 6, 0xFFFFAA);
        m_nameField->drawTextBox();
    }

    // 6. Control Hints Row
#if PLATFORM_XBOX
    const std::string buttons[] = {"A", "Y", "X", "White", "Black", "B"};
    const std::string actions[] = {
        isEs ? "Ver" : "Toggle",
        isEs ? "Nuevo" : "New",
        isEs ? "Borrar" : "Del",
        isEs ? "Nombre" : "Rename",
        isEs ? "Opciones" : "Opts",
        isEs ? "Salir" : "Back"
    };
    drawControlHintRow(mc, width, hintY, buttons, actions, 6);
#elif PLATFORM_PS2
    const std::string buttons[] = {"D-Pad", "Cross", "Triangle", "Square", "Circle"};
    const std::string actions[] = {
        isEs ? "Navegar" : "Navigate",
        isEs ? "Alternar" : "Toggle",
        isEs ? "Nuevo" : "New",
        isEs ? "Eliminar" : "Delete",
        isEs ? "Volver" : "Back"
    };
    drawControlHintRow(mc, width, hintY, buttons, actions, 5);
#else
    const std::string buttons[] = {"Up/Down", "Space", "N/R", "Del", "Esc"};
    const std::string actions[] = {
        isEs ? "Navegar" : "Navigate",
        isEs ? "Alternar" : "Toggle",
        isEs ? "Nuevo/Ren" : "New/Ren",
        isEs ? "Eliminar" : "Delete",
        isEs ? "Volver" : "Back"
    };
    drawControlHintRow(mc, width, hintY, buttons, actions, 5);
#endif

    GuiScreen::drawScreen(mouseX, mouseY, partialTick);
}
