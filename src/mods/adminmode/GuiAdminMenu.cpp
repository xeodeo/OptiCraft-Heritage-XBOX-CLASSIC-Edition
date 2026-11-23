#include "GuiAdminMenu.h"
#include "AdminModeMod.h"
#include "mods/ModManager.h"

#include "net/minecraft/src/Minecraft.h"
#include "net/minecraft/src/GameSettings.h"
#include "net/minecraft/src/GuiButton.h"
#include "net/minecraft/src/FontRenderer.h"
#include "net/minecraft/src/SoundManager.h"
#include "net/minecraft/src/StringTranslate.h"
#include "net/minecraft/src/World.h"
#include "net/minecraft/src/ControlIcon.h"
#include "net/minecraft/src/legacy/LegacyMenuWidgets.h"
#include "platform/PlatformConfig.h"
#include "platform/Input.h"

#if !PLATFORM_PS2 && !PLATFORM_WII && !PLATFORM_XBOX
#include "pc/lwjgl/Keyboard.h"
#endif

#include <algorithm>

namespace
{
bool adminMenuIsSpanish()
{
    StringTranslate *tr = StringTranslate::getInstance();
    return tr != nullptr && tr->getCurrentLanguage().rfind("es_", 0) == 0;
}

const int_t kDoneId = 900;
}

GuiAdminMenu::GuiAdminMenu(GuiScreen *parent, int playerIndex)
    : m_parent(parent)
    , m_playerIndex(playerIndex)
    , m_selectedIndex(0)
    , m_scrollOffset(0)
    , m_javaRows(0)
    , m_openLatch(true)
{
}

bool GuiAdminMenu::isLegacyMode() const
{
    return mc != nullptr && mc->gameSettings != nullptr && mc->gameSettings->legacyUI;
}

bool GuiAdminMenu::usesSpecializedMenuNavigation() const
{
#if PLATFORM_PS2 || PLATFORM_WII || PLATFORM_XBOX
    return true;
#else
    return isLegacyMode();
#endif
}

int_t GuiAdminMenu::itemCount() const
{
    return AdminModeMod::moduleCount();
}

std::string GuiAdminMenu::rowLabel(int_t idx, bool selected) const
{
    std::string label = AdminModeMod::moduleName(idx);
    AdminModeMod *mod = AdminModeMod::getInstance();
    if (mod != nullptr && AdminModeMod::optionCount(idx) > 0)
    {
        const std::string opt = AdminModeMod::optionLabel(idx, mod->moduleOption(idx));
        label += selected ? ("  < " + opt + " >") : ("  (" + opt + ")");
    }
    return label;
}

std::string GuiAdminMenu::statusLine(int_t &color) const
{
    const bool es = adminMenuIsSpanish();
    AdminModeMod *mod = AdminModeMod::getInstance();
    if (mod == nullptr || !ModManager::getInstance().isModEnabled("adminmode"))
    {
        color = static_cast<int_t>(0xffa02010u);
        return es ? "El mod esta apagado en el catalogo de Mods" : "The mod is switched off in the Mods catalog";
    }
    if (mc == nullptr || mc->theWorld == nullptr)
    {
        color = static_cast<int_t>(0xff505050u);
        return es ? "Fuera de partida: se aplica al entrar en un mundo" : "Not in a world: applies when you join one";
    }
    if (!mod->worldAllowed())
    {
        color = static_cast<int_t>(0xffa02010u);
        return es ? "Servidor ajeno: el modo admin no hace nada aqui" : "Someone else's server: admin mode does nothing here";
    }
    color = static_cast<int_t>(0xff207020u);
    if (mc->theWorld->multiplayerWorld)
        return es ? "Servidor propio (op): activo" : "Own server (op): active";
    return es ? "Mundo local: activo" : "Local world: active";
}

void GuiAdminMenu::click(float pitch)
{
    if (mc != nullptr && mc->sndManager != nullptr)
        mc->sndManager->playSoundFX("random.click", 1.0f, pitch);
}

void GuiAdminMenu::moveSelection(int_t delta)
{
    const int_t count = itemCount();
    m_selectedIndex = ((m_selectedIndex + delta) % count + count) % count;
    if (mc != nullptr && mc->sndManager != nullptr)
        mc->sndManager->playSoundFX("random.focus", 1.0f, 1.0f);
    refreshJavaButtons();
}

void GuiAdminMenu::toggleItem(int_t idx)
{
    AdminModeMod *mod = AdminModeMod::getInstance();
    if (mod == nullptr || idx < 0 || idx >= itemCount())
        return;
    const bool on = !mod->moduleState(idx);
    mod->setModuleState(idx, on);
    refreshJavaButtons();
    click(on ? 1.2f : 0.8f);
}

void GuiAdminMenu::changeOption(int_t idx, int_t dir)
{
    AdminModeMod *mod = AdminModeMod::getInstance();
    if (mod == nullptr || AdminModeMod::optionCount(idx) <= 0)
        return;
    mod->setModuleOption(idx, mod->moduleOption(idx) + (dir >= 0 ? 1 : -1));
    refreshJavaButtons();
    click(1.0f);
}

void GuiAdminMenu::allOff()
{
    if (AdminModeMod *mod = AdminModeMod::getInstance())
        mod->allOff();
    refreshJavaButtons();
    click(0.7f);
}

void GuiAdminMenu::refreshJavaButtons()
{
    if (isLegacyMode() || m_javaRows <= 0)
        return;
    m_scrollOffset = legacyMenuKeepVisible(m_selectedIndex, itemCount(), m_javaRows, m_scrollOffset);
    AdminModeMod *mod = AdminModeMod::getInstance();
    const bool es = adminMenuIsSpanish();
    for (GuiButton *btn : controlList)
    {
        if (btn == nullptr || btn->id < 0 || btn->id >= m_javaRows)
            continue;
        const int_t idx = m_scrollOffset + btn->id;
        btn->enabled = idx < itemCount();
        if (!btn->enabled)
        {
            btn->displayString = "";
            continue;
        }
        const bool on = mod != nullptr && mod->moduleState(idx);
        btn->displayString = rowLabel(idx, idx == m_selectedIndex) + ": " +
            (on ? (es ? "\xc2\xa7" "aSI" : "\xc2\xa7" "aON") : (es ? "NO" : "OFF"));
        btn->setKeyboardSelected(idx == m_selectedIndex);
    }
}

void GuiAdminMenu::initGui()
{
    m_openLatch = true;
    controlList.clear();
    m_javaRows = 0;
    if (!isLegacyMode())
    {
        const int_t top = 40;
        const int_t bottom = height - 58;
        m_javaRows = std::max<int_t>(3, (bottom - top) / 22);
        for (int_t i = 0; i < m_javaRows; ++i)
            controlList.push_back(new GuiButton(i, width / 2 - 140, top + i * 22, 280, 20, ""));
        StringTranslate *tr = StringTranslate::getInstance();
        controlList.push_back(new GuiButton(kDoneId, width / 2 - 100, height - 26, tr->translateKey("gui.done")));
        refreshJavaButtons();
    }
}

void GuiAdminMenu::closeScreen()
{
    if (AdminModeMod *mod = AdminModeMod::getInstance())
        mod->saveConfig();
    if (mc == nullptr)
        return;
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

void GuiAdminMenu::actionPerformed(GuiButton *button)
{
    if (button == nullptr || !button->enabled)
        return;
    if (button->id == kDoneId)
    {
        closeScreen();
        return;
    }
    if (button->id >= 0 && button->id < m_javaRows)
    {
        const int_t idx = m_scrollOffset + button->id;
        if (idx < itemCount())
        {
            m_selectedIndex = idx;
            toggleItem(idx);
        }
    }
}

void GuiAdminMenu::keyTyped(char_t c, int_t key)
{
    if (key == 1)
    {
        closeScreen();
        return;
    }
#if !PLATFORM_PS2 && !PLATFORM_WII && !PLATFORM_XBOX
    if (key == lwjgl::Keyboard::KEY_UP) { moveSelection(-1); return; }
    if (key == lwjgl::Keyboard::KEY_DOWN) { moveSelection(1); return; }
    if (key == lwjgl::Keyboard::KEY_LEFT) { changeOption(m_selectedIndex, -1); return; }
    if (key == lwjgl::Keyboard::KEY_RIGHT) { changeOption(m_selectedIndex, 1); return; }
    if (key == lwjgl::Keyboard::KEY_RETURN || key == lwjgl::Keyboard::KEY_SPACE) { toggleItem(m_selectedIndex); return; }
    if (key == lwjgl::Keyboard::KEY_DELETE) { allOff(); return; }
#endif
    GuiScreen::keyTyped(c, key);
}

void GuiAdminMenu::handleSpecializedMenuInput()
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
        moveSelection(-1);
    else if (pad.pressed & PLATFORM_TEXT_DOWN)
        moveSelection(1);
    else if (pad.pressed & PLATFORM_TEXT_PREV_PAGE)
        moveSelection(-std::min<int_t>(m_selectedIndex, 8));
    else if (pad.pressed & PLATFORM_TEXT_NEXT_PAGE)
        moveSelection(std::min<int_t>(itemCount() - 1 - m_selectedIndex, 8));
    else if (pad.pressed & PLATFORM_TEXT_LEFT)
        changeOption(m_selectedIndex, -1);
    else if (pad.pressed & PLATFORM_TEXT_RIGHT)
        changeOption(m_selectedIndex, 1);
    else if (pad.pressed & PLATFORM_TEXT_TYPE)
        toggleItem(m_selectedIndex);
    else if (pad.pressed & PLATFORM_TEXT_BACK)
        allOff();
#endif
}

void GuiAdminMenu::drawScreen(int_t mouseX, int_t mouseY, float_t partialTick)
{
    const bool es = adminMenuIsSpanish();
    const std::string title = es ? "Modo admin (pruebas)" : "Admin mode (testing)";
    int_t statusColor = 0;
    const std::string status = statusLine(statusColor);
    AdminModeMod *mod = AdminModeMod::getInstance();
    const int_t count = itemCount();
    m_selectedIndex = std::clamp<int_t>(m_selectedIndex, 0, count - 1);

    std::string desc = AdminModeMod::moduleDesc(m_selectedIndex);

    if (!isLegacyMode())
    {
        drawDefaultBackground();
        drawCenteredString(fontRenderer, title, width / 2, 10, 0xFFFFFF);
        drawCenteredString(fontRenderer, status, width / 2, 24, statusColor & 0xFFFFFF);
        GuiScreen::drawScreen(mouseX, mouseY, partialTick);
        drawCenteredString(fontRenderer, desc, width / 2, height - 54, 0xCCCCCC);
        drawCenteredString(fontRenderer,
            es ? "Â§" "7A: activar   Izq/Der: ajuste   X: apagar todo   B: volver"
               : "Â§" "7A: toggle   Left/Right: setting   X: all off   B: back",
            width / 2, height - 42, 0xAAAAAA);
        return;
    }

    legacyDrawMenuBackdrop(mc, width, height, partialTick);
    const bool compact = (height < 180);
    const int_t headerH = compact ? 24 : 32;
    const LegacyMenuLayout l = legacyMenuLayout(mc, width, height, 300, count, headerH, compact ? 12 : 22);
    legacyDrawMenuPanel(l.panelX, l.panelY, l.panelW, l.panelH);
    legacyDrawPanelTitle(fontRenderer, title, l.panelX + l.panelW / 2, l.titleY);
    const std::string st = legacyFitText(fontRenderer, status, l.contentW);
    legacyDrawPanelText(fontRenderer, st, l.panelX + (l.panelW - fontRenderer->getStringWidth(st)) / 2,
        l.titleY + (compact ? 9 : 12), statusColor);

    m_scrollOffset = legacyMenuKeepVisible(m_selectedIndex, count, l.visibleRows, m_scrollOffset);
    for (int_t vi = 0; vi < l.visibleRows; ++vi)
    {
        const int_t idx = m_scrollOffset + vi;
        if (idx >= count)
            break;
        const bool sel = (idx == m_selectedIndex);
        const bool on = mod != nullptr && mod->moduleState(idx);
        legacyDrawMenuCheckbox(mc, l.contentX + 4, l.rowY(vi), l.contentW - 12, l.rowH,
            legacyFitText(fontRenderer, rowLabel(idx, sel), l.contentW - 40), on, sel);
    }
    const int_t listH = l.visibleRows * l.rowH + (l.visibleRows - 1) * l.rowGap;
    legacyDrawMenuScrollbar(l.panelX + l.panelW - 10, l.firstRowY, listH, m_scrollOffset, l.visibleRows, count);

    legacyDrawPanelTextWrapped(fontRenderer, desc, l.panelX + l.panelW / 2, l.footerY + 1, l.contentW,
        static_cast<int_t>(0xff404040u), compact ? 1 : 2);

#if PLATFORM_XBOX
    const std::string btns[] = {"D-Pad", "A", "X", "B"};
    const std::string acts[] = {
        es ? "Mover / ajuste" : "Move / setting",
        es ? "Activar" : "Toggle",
        es ? "Apagar todo" : "All off",
        es ? "Volver" : "Back"};
    drawControlHintRow(mc, width, l.hintY, btns, acts, 4);
#else
    const std::string btns[] = {"Arrows", "Enter", "Del", "Esc"};
    const std::string acts[] = {
        es ? "Mover / ajuste" : "Move / setting",
        es ? "Activar" : "Toggle",
        es ? "Apagar todo" : "All off",
        es ? "Volver" : "Back"};
    drawControlHintRow(mc, width, l.hintY, btns, acts, 4);
#endif
}
