#include "GuiCompiledMods.h"
#include "GuiLoadModsMenu.h"
#include "GuiTexturePacks.h"
#include "ModManager.h"

#include "GuiButton.h"
#include "GuiTextField.h"
#include "Minecraft.h"
#include "GameSettings.h"
#include "FontRenderer.h"
#include "SoundManager.h"
#include "StringTranslate.h"
#include "ControlIcon.h"
#include "legacy/LegacyMenuHints.h"
#include "legacy/LegacyOptionsLayout.h"
#include "legacy/LegacyOptionsPanel.h"
#include "legacy/LegacyPanorama.h"
#include "platform/PlatformConfig.h"
#include "platform/Input.h"

#if defined(PS2_PLATFORM) || defined(WII_PLATFORM) || defined(XBOX_PLATFORM)
#include "net/minecraft/src/VirtualKeyboard.h"
#endif

#if !PLATFORM_PS2 && !PLATFORM_WII && !PLATFORM_XBOX
#include "pc/lwjgl/Keyboard.h"
#endif

#include <algorithm>
#include <cctype>
#include <cstdio>

namespace
{
bool catalogIsSpanish()
{
    StringTranslate *tr = StringTranslate::getInstance();
    return (tr != nullptr && tr->getCurrentLanguage().rfind("es_", 0) == 0);
}

std::string toLowerCopy(const std::string &s)
{
    std::string out = s;
    for (char &ch : out)
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    return out;
}

std::string fitText(FontRenderer *fr, const std::string &txt, int_t maxW)
{
    if (fr == nullptr || maxW <= 8 || fr->getStringWidth(txt) <= maxW)
        return txt;
    std::string out = txt;
    while (!out.empty() && fr->getStringWidth(out + "...") > maxW)
        out.pop_back();
    return out + "...";
}
} // namespace

// Draws a crisp procedural pixel-art thumbnail/icon for each built-in mod.
void GuiCompiledMods::drawModIcon(Gui *gui, const std::string &modId, int_t x, int_t y, int_t size)
{
    if (gui == nullptr || size < 12)
        return;

    auto cell = [&](int_t gx, int_t gy, int_t gw, int_t gh, int_t color)
    {
        const int_t x0 = x + (gx * size) / 8;
        const int_t y0 = y + (gy * size) / 8;
        const int_t x1 = x + ((gx + gw) * size) / 8;
        const int_t y1 = y + ((gy + gh) * size) / 8;
        gui->drawRect(x0, y0, x1, y1, color);
    };

    // Frame border
    gui->drawRect(x - 1, y - 1, x + size + 1, y + size + 1, static_cast<int_t>(0xFF181818u));

    if (modId == "reiminimap")
    {
        // Green/blue terrain minimap with red player marker
        cell(0, 0, 8, 8, static_cast<int_t>(0xFF3E8C38u));
        cell(4, 0, 4, 4, static_cast<int_t>(0xFF306EE8u));
        cell(0, 5, 3, 3, static_cast<int_t>(0xFFD4C480u));
        cell(3, 3, 2, 2, static_cast<int_t>(0xFFFF2222u));
        cell(6, 5, 1, 1, static_cast<int_t>(0xFF00FF88u));
    }
    else if (modId == "herobrine")
    {
        // Classic Steve head with glowing pure white eyes!
        cell(0, 0, 8, 8, static_cast<int_t>(0xFFC69674u));
        cell(0, 0, 8, 2, static_cast<int_t>(0xFF3B2414u));
        cell(0, 2, 1, 1, static_cast<int_t>(0xFF3B2414u));
        cell(7, 2, 1, 1, static_cast<int_t>(0xFF3B2414u));
        cell(1, 4, 2, 1, static_cast<int_t>(0xFFFFFFFFu)); // Glowing white left eye
        cell(5, 4, 2, 1, static_cast<int_t>(0xFFFFFFFFu)); // Glowing white right eye
        cell(3, 5, 2, 1, static_cast<int_t>(0xFF9E6B4Eu));
        cell(2, 6, 4, 2, static_cast<int_t>(0xFF7A4B30u));
    }
    else if (modId == "chococraft")
    {
        // Yellow Chocobo face with crest & orange beak
        cell(0, 0, 8, 8, static_cast<int_t>(0xFF284828u));
        cell(1, 1, 6, 6, static_cast<int_t>(0xFFF4CC28u));
        cell(2, 0, 4, 2, static_cast<int_t>(0xFFFFEE66u));
        cell(2, 3, 1, 1, static_cast<int_t>(0xFF183868u));
        cell(5, 3, 1, 1, static_cast<int_t>(0xFF183868u));
        cell(2, 5, 4, 2, static_cast<int_t>(0xFFE88620u));
    }
    else if (modId == "toomanyitems")
    {
        // Inventory / chest item grid
        cell(0, 0, 8, 8, static_cast<int_t>(0xFF8B5A2Bu));
        cell(1, 1, 6, 6, static_cast<int_t>(0xFF5A3818u));
        cell(2, 2, 2, 2, static_cast<int_t>(0xFF44EEEEu));
        cell(4, 2, 2, 2, static_cast<int_t>(0xFFFFD700u));
        cell(2, 4, 2, 2, static_cast<int_t>(0xFF44DD44u));
        cell(4, 4, 2, 2, static_cast<int_t>(0xFFFF4444u));
    }
    else if (modId == "xray")
    {
        // Dark cave with glowing cyan diamond ore
        cell(0, 0, 8, 8, static_cast<int_t>(0xFF181A24u));
        cell(1, 1, 2, 2, static_cast<int_t>(0xFF48F0F0u));
        cell(5, 2, 2, 2, static_cast<int_t>(0xFF48F0F0u));
        cell(2, 5, 2, 2, static_cast<int_t>(0xFFFFCC22u));
        cell(5, 5, 2, 2, static_cast<int_t>(0xFF48F0F0u));
    }
    else if (modId == "dynamiclights")
    {
        // Glowing torch with warm light halo
        cell(0, 0, 8, 8, static_cast<int_t>(0xFF282014u));
        cell(1, 1, 6, 5, static_cast<int_t>(0xFF584018u));
        cell(2, 1, 4, 3, static_cast<int_t>(0xFFFFAA22u));
        cell(3, 1, 2, 2, static_cast<int_t>(0xFFFFFF88u));
        cell(3, 4, 2, 4, static_cast<int_t>(0xFF8B5A2Bu));
    }
    else if (modId == "strongholdlocator")
    {
        // Eye of Ender
        cell(0, 0, 8, 8, static_cast<int_t>(0xFF182A24u));
        cell(1, 1, 6, 6, static_cast<int_t>(0xFF2E9C68u));
        cell(2, 2, 4, 4, static_cast<int_t>(0xFF66EEAAu));
        cell(3, 2, 2, 4, static_cast<int_t>(0xFF102018u));
    }
    else if (modId == "villagelocator")
    {
        // Village house & emerald
        cell(0, 0, 8, 8, static_cast<int_t>(0xFF4A90D8u));
        cell(0, 5, 8, 3, static_cast<int_t>(0xFF4C9A38u));
        cell(1, 2, 6, 2, static_cast<int_t>(0xFF8C5428u));
        cell(2, 4, 4, 3, static_cast<int_t>(0xFFD8C292u));
        cell(3, 5, 2, 2, static_cast<int_t>(0xFF583418u));
    }
    else if (modId == "serverutils")
    {
        // Ping bars + info badge
        cell(0, 0, 8, 8, static_cast<int_t>(0xFF223044u));
        cell(1, 5, 1, 2, static_cast<int_t>(0xFF44EE44u));
        cell(3, 3, 1, 4, static_cast<int_t>(0xFF44EE44u));
        cell(5, 1, 1, 6, static_cast<int_t>(0xFF44EE44u));
        cell(6, 1, 1, 1, static_cast<int_t>(0xFF66EEFFu));
    }
    else
    {
        // Generic mod cube
        cell(0, 0, 8, 8, static_cast<int_t>(0xFF445566u));
        cell(2, 2, 4, 4, static_cast<int_t>(0xFF88AACCu));
    }
}

GuiCompiledMods::GuiCompiledMods(GuiScreen *parent)
    : parentScreen(parent)
{
}

GuiCompiledMods::~GuiCompiledMods()
{
    delete searchField;
    searchField = nullptr;
}

bool GuiCompiledMods::isLegacyMode() const
{
    return mc != nullptr && mc->gameSettings != nullptr && mc->gameSettings->legacyUI;
}

bool GuiCompiledMods::usesSpecializedMenuNavigation() const
{
    if (searching)
        return false;
#if PLATFORM_PS2 || PLATFORM_WII || PLATFORM_XBOX
    return true;
#else
    return isLegacyMode();
#endif
}

void GuiCompiledMods::rebuildFilteredList()
{
    allEntries = ModManager::getInstance().getCatalogEntries();
    filteredEntries.clear();
    const std::string query = searchField ? toLowerCopy(searchField->getText()) : "";

    for (const auto &entry : allEntries)
    {
        if (filterTab == FilterTab::Active && !entry.enabled)
            continue;
        if (filterTab == FilterTab::Available && entry.enabled)
            continue;
        if (!query.empty())
        {
            const std::string hay = toLowerCopy(entry.name + " " + entry.id + " " + entry.description + " " + entry.author);
            if (hay.find(query) == std::string::npos)
                continue;
        }
        filteredEntries.push_back(entry);
    }

    if (filteredEntries.empty())
        selectedIndex = -1;
    else
        selectedIndex = std::clamp<int_t>(selectedIndex < 0 ? 0 : selectedIndex, 0, static_cast<int_t>(filteredEntries.size()) - 1);
    refreshActionButtons();
}

void GuiCompiledMods::refreshActionButtons()
{
    const bool es = catalogIsSpanish();
    const bool valid = (selectedIndex >= 0 && selectedIndex < static_cast<int_t>(filteredEntries.size()));
    if (installBtn != nullptr)
    {
        installBtn->enabled = valid && filteredEntries[selectedIndex].installed;
        if (valid)
        {
            installBtn->displayString = filteredEntries[selectedIndex].enabled
                ? (es ? "Desactivar" : "Disable")
                : (es ? "Instalar / Activar" : "Install / Enable");
        }
        else
        {
            installBtn->displayString = es ? "Activar" : "Enable";
        }
    }
    if (configBtn != nullptr)
        configBtn->enabled = valid && filteredEntries[selectedIndex].hasConfig;
    if (filterBtn != nullptr)
    {
        if (filterTab == FilterTab::All) filterBtn->displayString = es ? "Ver: Todos" : "Tab: All";
        else if (filterTab == FilterTab::Active) filterBtn->displayString = es ? "Ver: Activos" : "Tab: Active";
        else filterBtn->displayString = es ? "Ver: Disponibles" : "Tab: Available";
    }
}

void GuiCompiledMods::initGui()
{
    openLatch = true;
    searching = false;
    controlList.clear();
    installBtn = nullptr;
    configBtn = nullptr;
    searchBtn = nullptr;
    filterBtn = nullptr;

    delete searchField;
    const int_t sfW = std::min<int_t>(140, width / 2 - 12);
    searchField = new GuiTextField(this, fontRenderer, width / 2 - sfW / 2, 22, sfW, 14, "");
    searchField->setMaxStringLength(24);
    searchField->setFocused(false);

    if (!isLegacyMode())
    {
        const bool es = catalogIsSpanish();
        const int_t btnY1 = height - 46;
        const int_t btnY2 = height - 24;
        const int_t w3 = std::min<int_t>(100, (width - 24) / 3);
        const int_t x0 = width / 2 - (w3 * 3 + 8) / 2;
        installBtn = new GuiButton(10, x0, btnY1, w3, 20, es ? "Activar" : "Enable");
        configBtn  = new GuiButton(11, x0 + w3 + 4, btnY1, w3, 20, es ? "Opciones" : "Options");
        filterBtn  = new GuiButton(12, x0 + (w3 + 4) * 2, btnY1, w3, 20, es ? "Ver: Todos" : "Tab: All");
        controlList.push_back(installBtn);
        controlList.push_back(configBtn);
        controlList.push_back(filterBtn);

        searchBtn = new GuiButton(13, x0, btnY2, w3, 20, es ? "Buscar" : "Search");
        controlList.push_back(searchBtn);
        controlList.push_back(new GuiButton(14, x0 + w3 + 4, btnY2, w3, 20, es ? "Cargar .och" : "Load .och"));
        controlList.push_back(new GuiButton(200, x0 + (w3 + 4) * 2, btnY2, w3, 20, es ? "Hecho" : "Done"));
    }

    rebuildFilteredList();
}

void GuiCompiledMods::onGuiClosed()
{
#if defined(PS2_PLATFORM) || defined(WII_PLATFORM) || defined(XBOX_PLATFORM)
    VirtualKeyboard::instance().releaseFocus();
#endif
    if (searchField != nullptr)
        searchField->setFocused(false);
    GuiScreen::onGuiClosed();
}

void GuiCompiledMods::updateScreen()
{
    GuiScreen::updateScreen();
    if (searching && searchField != nullptr)
        searchField->updateCursorCounter();
}

void GuiCompiledMods::toggleSelectedMod()
{
    if (selectedIndex < 0 || selectedIndex >= static_cast<int_t>(filteredEntries.size()))
        return;
    const CompiledModInfo &entry = filteredEntries[selectedIndex];
    if (!entry.installed)
        return;

    const bool newState = !entry.enabled;
    ModManager::getInstance().setModEnabled(entry.id, newState);
    if (mc != nullptr && mc->sndManager != nullptr)
        mc->sndManager->playSoundFX("random.click", 1.0f, newState ? 1.15f : 0.85f);
    rebuildFilteredList();
}

void GuiCompiledMods::openSelectedModConfig()
{
    if (selectedIndex < 0 || selectedIndex >= static_cast<int_t>(filteredEntries.size()))
        return;
    IMod *mod = ModManager::getInstance().getMod(filteredEntries[selectedIndex].id);
    if (mod == nullptr || !mod->hasConfigScreen())
        return;
    GuiScreen *cfg = mod->createConfigScreen(this);
    if (cfg != nullptr && mc != nullptr)
    {
        if (mc->sndManager != nullptr)
            mc->sndManager->playSoundFX("random.click", 1.0f, 1.0f);
        mc->displayGuiScreen(cfg);
    }
}

void GuiCompiledMods::beginSearch()
{
    if (searchField == nullptr)
        return;
    searching = true;
    searchField->setFocused(true);
    if (mc != nullptr && mc->sndManager != nullptr)
        mc->sndManager->playSoundFX("random.click", 1.0f, 1.0f);
}

void GuiCompiledMods::endSearch()
{
    searching = false;
    openLatch = true;
    if (searchField != nullptr)
        searchField->setFocused(false);
#if defined(PS2_PLATFORM) || defined(WII_PLATFORM) || defined(XBOX_PLATFORM)
    VirtualKeyboard::instance().releaseFocus();
#endif
    rebuildFilteredList();
}

void GuiCompiledMods::cycleFilter(int_t dir)
{
    int next = (static_cast<int>(filterTab) + (dir >= 0 ? 1 : 2)) % 3;
    filterTab = static_cast<FilterTab>(next);
    scrollOffset = 0;
    selectedIndex = 0;
    if (mc != nullptr && mc->sndManager != nullptr)
        mc->sndManager->playSoundFX("random.click", 1.0f, 1.0f);
    rebuildFilteredList();
}

void GuiCompiledMods::moveSelection(int_t delta)
{
    if (filteredEntries.empty())
        return;
    const int_t count = static_cast<int_t>(filteredEntries.size());
    selectedIndex = (selectedIndex + delta + count) % count;
    refreshActionButtons();
    if (mc != nullptr && mc->sndManager != nullptr)
        mc->sndManager->playSoundFX("random.focus", 1.0f, 1.0f);
}

void GuiCompiledMods::clampSelection(int_t visibleRows)
{
    const int_t count = static_cast<int_t>(filteredEntries.size());
    if (count <= 0)
    {
        selectedIndex = -1;
        scrollOffset = 0;
        return;
    }
    selectedIndex = std::clamp<int_t>(selectedIndex, 0, count - 1);
    if (visibleRows < 1) visibleRows = 1;
    if (selectedIndex < scrollOffset) scrollOffset = selectedIndex;
    if (selectedIndex >= scrollOffset + visibleRows) scrollOffset = selectedIndex - visibleRows + 1;
    scrollOffset = std::clamp<int_t>(scrollOffset, 0, std::max<int_t>(0, count - visibleRows));
}

void GuiCompiledMods::actionPerformed(GuiButton *button)
{
    if (button == nullptr || !button->enabled)
        return;
    if (button->id == 10) toggleSelectedMod();
    else if (button->id == 11) openSelectedModConfig();
    else if (button->id == 12) cycleFilter(1);
    else if (button->id == 13) beginSearch();
    else if (button->id == 14 && mc != nullptr) mc->displayGuiScreen(new GuiLoadModsMenu(this));
    else if (button->id == 200 && mc != nullptr)
    {
        ModManager::getInstance().save();
        mc->displayGuiScreen(parentScreen);
    }
}

void GuiCompiledMods::mouseClicked(int_t x, int_t y, int_t button)
{
    if (searchField != nullptr)
        searchField->mouseClicked(x, y, button);

    // Hit-test visible catalog cards in Java UI
    if (!isLegacyMode() && button == 0)
    {
        const bool wide = (width >= 420);
        const int_t listX = wide ? 12 : (width - std::min<int_t>(300, width - 12)) / 2;
        const int_t listW = wide ? (width * 56 / 100 - 16) : std::min<int_t>(300, width - 12);
        const int_t listY = 42;
        const int_t rowH = 26;
        const int_t listH = height - 52 - listY;
        const int_t vis = std::max<int_t>(1, listH / rowH);
        if (x >= listX && x < listX + listW && y >= listY && y < listY + vis * rowH)
        {
            const int_t row = (y - listY) / rowH;
            const int_t idx = scrollOffset + row;
            if (idx >= 0 && idx < static_cast<int_t>(filteredEntries.size()))
            {
                if (idx == selectedIndex && x >= listX + listW - 70)
                    toggleSelectedMod();
                else
                {
                    selectedIndex = idx;
                    refreshActionButtons();
                    if (mc != nullptr && mc->sndManager != nullptr)
                        mc->sndManager->playSoundFX("random.click", 1.0f, 1.0f);
                }
                return;
            }
        }
    }

    GuiScreen::mouseClicked(x, y, button);
}

void GuiCompiledMods::keyTyped(char_t c, int_t key)
{
    if (searching && searchField != nullptr)
    {
        if (key == 1 || key == 28 || c == '\r' || c == '\n')
        {
            endSearch();
            return;
        }
        searchField->textboxKeyTyped(c, key);
        rebuildFilteredList();
        return;
    }

    if (key == 1 && mc != nullptr)
    {
        ModManager::getInstance().save();
        mc->displayGuiScreen(parentScreen);
        return;
    }
#if !PLATFORM_PS2 && !PLATFORM_WII && !PLATFORM_XBOX
    if (key == lwjgl::Keyboard::KEY_UP) { moveSelection(-1); return; }
    if (key == lwjgl::Keyboard::KEY_DOWN) { moveSelection(1); return; }
    if (key == lwjgl::Keyboard::KEY_RETURN || key == lwjgl::Keyboard::KEY_SPACE) { toggleSelectedMod(); return; }
    if (key == lwjgl::Keyboard::KEY_F) { beginSearch(); return; }
    if (key == lwjgl::Keyboard::KEY_TAB) { cycleFilter(1); return; }
#endif
    GuiScreen::keyTyped(c, key);
}

void GuiCompiledMods::handleSpecializedMenuInput()
{
    if (searching)
        return;

#if PLATFORM_PS2 || PLATFORM_WII || PLATFORM_XBOX
    const PlatformTextInputSnapshot pad = platformTextInputSnapshot(platformMenuPad());
    if (!pad.connected)
        return;

    if (openLatch)
    {
        if ((pad.held & (PLATFORM_TEXT_TYPE | PLATFORM_TEXT_CLOSE | PLATFORM_TEXT_BACK | PLATFORM_TEXT_SHIFT |
                         PLATFORM_TEXT_TAB_LEFT | PLATFORM_TEXT_TAB_RIGHT)) == 0)
            openLatch = false;
        return;
    }

    if (pad.pressed & PLATFORM_TEXT_CLOSE)
    {
        ModManager::getInstance().save();
        if (mc != nullptr && mc->sndManager != nullptr)
            mc->sndManager->playSoundFX("random.back", 1.0f, 1.0f);
        if (mc != nullptr)
            mc->displayGuiScreen(parentScreen);
        return;
    }

    if (pad.pressed & PLATFORM_TEXT_UP)         { moveSelection(-1); return; }
    if (pad.pressed & PLATFORM_TEXT_DOWN)       { moveSelection(1);  return; }
    if (pad.pressed & PLATFORM_TEXT_TYPE)       { toggleSelectedMod(); return; }
    if (pad.pressed & PLATFORM_TEXT_SHIFT)      { openSelectedModConfig(); return; }
    if (pad.pressed & PLATFORM_TEXT_BACK)       { beginSearch(); return; }
    if (pad.pressed & PLATFORM_TEXT_TAB_LEFT)   { cycleFilter(-1); return; }
    if (pad.pressed & PLATFORM_TEXT_TAB_RIGHT)  { cycleFilter(1);  return; }
#endif
}

void GuiCompiledMods::drawScreen(int_t mouseX, int_t mouseY, float_t partialTick)
{
    const bool es = catalogIsSpanish();
    const bool legacy = isLegacyMode();
    const bool inWorld = (mc != nullptr && mc->theWorld != nullptr);
    const bool isSplit = (mc != nullptr && mc->isSplitScreenActive());

    // 1. Background
    if (legacy)
    {
        if (!inWorld && mc != nullptr && legacyDrawPanorama(mc, width, height, 0, partialTick, zLevel))
        {
        }
        else
        {
            drawGradientRect(0, 0, width, height, static_cast<int_t>(0xC0101010u), static_cast<int_t>(0xD8101010u));
        }
    }
    else
    {
        drawDefaultBackground();
    }

    const int_t hintY = legacyHintRowY(height, isSplit);
    const int_t topY = legacy ? 8 : 6;
    const int_t bottomLimit = legacy ? (hintY - 6) : (height - 52);
    const int_t panelW = std::min<int_t>(width - 12, legacy ? 460 : 520);
    const int_t panelX = (width - panelW) / 2;
    const int_t panelH = std::max<int_t>(110, bottomLimit - topY);

    if (legacy)
    {
        LegacyOptionsLayout layout{};
        layout.panelX = panelX;
        layout.panelY = topY;
        layout.panelWidth = panelW;
        layout.panelHeight = panelH;
        LegacyOptionsPanel panel;
        panel.draw(layout);
    }

    // 2. Title + Filter Tabs + Search Box
    const int_t headerY = topY + 6;
    drawString(fontRenderer, es ? "Catalogo de Mods" : "Mod Catalog", panelX + 8, headerY, 0xFFFF88);

    // Filter tabs on right of header
    const char *tabNames[3] = {
        es ? "Todos" : "All",
        es ? "Activos" : "Active",
        es ? "Disp." : "Avail."
    };
    int_t tabRight = panelX + panelW - 8;
    for (int t = 2; t >= 0; --t)
    {
        const bool activeTab = (static_cast<int>(filterTab) == t);
        const int_t tw = fontRenderer->getStringWidth(tabNames[t]) + 8;
        const int_t tx = tabRight - tw;
        drawRect(tx, headerY - 1, tabRight, headerY + 10,
            activeTab ? static_cast<int_t>(0xE02A68A8u) : static_cast<int_t>(0x80202020u));
        drawCenteredString(fontRenderer, tabNames[t], tx + tw / 2, headerY + 1, activeTab ? 0xFFFFAA : 0xAAAAAA);
        tabRight = tx - 3;
    }

    // Search bar row
    const int_t barY = headerY + 13;
    const int_t barW = panelW - 16;
    const std::string qText = searchField ? searchField->getText() : "";
    drawRect(panelX + 8, barY, panelX + 8 + barW, barY + 13,
        searching ? static_cast<int_t>(0xE0203048u) : static_cast<int_t>(0xB0141414u));
    drawRect(panelX + 8, barY, panelX + 8 + barW, barY + 1,
        searching ? static_cast<int_t>(0xFF66CCFFu) : static_cast<int_t>(0xFF555555u));
    drawRect(panelX + 8, barY + 12, panelX + 8 + barW, barY + 13,
        searching ? static_cast<int_t>(0xFF66CCFFu) : static_cast<int_t>(0xFF555555u));

    std::string searchLabel;
    if (qText.empty())
        searchLabel = searching ? (es ? "Escribe para buscar..._" : "Type to search..._")
                                : (es ? "[X] Buscar mod por nombre..." : "[X] Search mod by name...");
    else
        searchLabel = (es ? "Buscar: " : "Search: ") + qText + (searching ? "_" : "");
    drawString(fontRenderer, fitText(fontRenderer, searchLabel, barW - 8),
        panelX + 12, barY + 3, qText.empty() && !searching ? 0x888888 : 0xFFFFFF);

    // 3. Two-column Store Layout on 720p (width >= 420), compact list + preview footer on 480p!
    const bool wide = (width >= 420);
    const int_t contentY = barY + 16;
    const int_t contentBottom = topY + panelH - 6;
    const int_t listX = panelX + 8;
    const int_t listW = wide ? (panelW * 58 / 100 - 10) : (panelW - 16);
    const int_t listH = wide ? (contentBottom - contentY) : (contentBottom - contentY - 26);
    const int_t rowH = 26;
    const int_t visibleRows = std::max<int_t>(1, listH / rowH);
    clampSelection(visibleRows);

    if (filteredEntries.empty())
    {
        drawCenteredString(fontRenderer,
            es ? "Ningun mod coincide con la busqueda." : "No mods match your search.",
            listX + listW / 2, contentY + 22, 0xAAAAAA);
    }
    else
    {
        const int_t count = static_cast<int_t>(filteredEntries.size());
        const int_t rowsToDraw = std::min<int_t>(visibleRows, count - scrollOffset);
        for (int_t r = 0; r < rowsToDraw; ++r)
        {
            const int_t idx = scrollOffset + r;
            const CompiledModInfo &entry = filteredEntries[idx];
            const int_t ry = contentY + r * rowH;
            const bool sel = (idx == selectedIndex);

            drawRect(listX, ry, listX + listW - 5, ry + rowH - 2,
                sel ? static_cast<int_t>(0xD83A5288u) : static_cast<int_t>(0x901E1E1Eu));
            if (sel)
                drawRect(listX, ry, listX + 3, ry + rowH - 2, static_cast<int_t>(0xFFFFDD44u));

            // 16x16 pixel-art icon
            drawModIcon(this, entry.id, listX + 6, ry + 4, 16);

            // Install/Active 1-click pill on right
            const int_t pillW = 54;
            const int_t pillH = 14;
            const int_t pillX = listX + listW - 9 - pillW;
            const int_t pillY = ry + 5;
            const int_t pillBg = entry.enabled ? static_cast<int_t>(0xCC186818u) : static_cast<int_t>(0xCC224468u);
            const int_t pillBorder = entry.enabled ? static_cast<int_t>(0xFF44DD44u) : static_cast<int_t>(0xFF55AAFFu);
            drawRect(pillX, pillY, pillX + pillW, pillY + pillH, pillBg);
            drawRect(pillX, pillY, pillX + pillW, pillY + 1, pillBorder);
            drawRect(pillX, pillY + pillH - 1, pillX + pillW, pillY + pillH, pillBorder);
            drawCenteredString(fontRenderer,
                entry.enabled ? (es ? "ACTIVO" : "ACTIVE") : (es ? "INSTALAR" : "INSTALL"),
                pillX + pillW / 2, pillY + 3, entry.enabled ? 0xAAFFAA : 0xAADDFF);

            const int_t txtX = listX + 26;
            const int_t txtMaxW = (pillX - 4) - txtX;
            std::string title = entry.name + " \xc2\xa7" "7" + entry.version;
            drawString(fontRenderer, fitText(fontRenderer, title, txtMaxW), txtX, ry + 3, sel ? 0xFFFFAA : 0xFFFFFF);
            drawString(fontRenderer, fitText(fontRenderer, entry.description, txtMaxW), txtX, ry + 13, sel ? 0xDDDDDD : 0x999999);
        }

        // Scrollbar
        if (count > visibleRows)
        {
            const int_t tx = listX + listW - 3;
            const int_t th = visibleRows * rowH - 2;
            drawRect(tx, contentY, tx + 3, contentY + th, static_cast<int_t>(0x70000000u));
            const int_t maxScr = std::max<int_t>(1, count - visibleRows);
            const int_t thumbH = std::max<int_t>(10, (th * visibleRows) / count);
            const int_t thumbY = contentY + ((th - thumbH) * scrollOffset) / maxScr;
            drawRect(tx, thumbY, tx + 3, thumbY + thumbH, static_cast<int_t>(0xFFDDDDDDu));
        }

        // 4. Store Detail Card (side card in 720p, compact strip in 480p)
        if (selectedIndex >= 0 && selectedIndex < count)
        {
            const CompiledModInfo &selEntry = filteredEntries[selectedIndex];
            if (wide)
            {
                const int_t detX = listX + listW + 4;
                const int_t detW = panelX + panelW - 8 - detX;
                const int_t detH = contentBottom - contentY;
                drawRect(detX, contentY, detX + detW, contentY + detH, static_cast<int_t>(0xB8141820u));
                drawRect(detX, contentY, detX + detW, contentY + 1, static_cast<int_t>(0xFF556677u));
                drawRect(detX, contentY + detH - 1, detX + detW, contentY + detH, static_cast<int_t>(0xFF556677u));

                // Large 32x32 mod icon preview
                drawModIcon(this, selEntry.id, detX + 8, contentY + 8, 32);
                const int_t infoX = detX + 46;
                const int_t infoW = detW - 52;
                drawString(fontRenderer, fitText(fontRenderer, selEntry.name, infoW), infoX, contentY + 10, 0xFFFF88);
                drawString(fontRenderer, selEntry.version + " - " + selEntry.author, infoX, contentY + 21, 0xAAAAAA);
                drawString(fontRenderer,
                    selEntry.enabled ? (es ? "\xc2\xa7" "aInstalado y activo" : "\xc2\xa7" "aInstalled & Active")
                                     : (es ? "\xc2\xa7" "bListo en 1 clic" : "\xc2\xa7" "b1-Click Ready"),
                    infoX, contentY + 32, 0xFFFFFF);

                // Wrapped description lines
                int_t dy = contentY + 48;
                std::string rem = selEntry.description;
                while (!rem.empty() && dy < contentY + detH - 26)
                {
                    std::string line = fontRenderer->trimStringToWidth(rem, detW - 16);
                    if (line.empty()) break;
                    if (line.size() < rem.size())
                    {
                        size_t sp = line.rfind(' ');
                        if (sp != std::string::npos && sp > 4)
                            line = line.substr(0, sp);
                    }
                    drawString(fontRenderer, line, detX + 8, dy, 0xDDDDDD);
                    rem.erase(0, line.size());
                    while (!rem.empty() && rem.front() == ' ') rem.erase(rem.begin());
                    dy += 10;
                }

                // Bottom action badge inside detail card
                const int_t actY = contentY + detH - 20;
                drawRect(detX + 8, actY, detX + detW - 8, actY + 14,
                    selEntry.enabled ? static_cast<int_t>(0xCC782020u) : static_cast<int_t>(0xCC1A7828u));
                drawCenteredString(fontRenderer,
                    selEntry.enabled ? (es ? "[A] Desactivar" : "[A] Disable")
                                     : (es ? "[A] Instalar / Activar" : "[A] Install / Enable"),
                    detX + detW / 2, actY + 3, 0xFFFFFF);
            }
            else
            {
                // Compact 480p footer preview bar
                const int_t fy = contentBottom - 23;
                drawRect(listX, fy, listX + listW, contentBottom, static_cast<int_t>(0xB8141820u));
                drawString(fontRenderer, fitText(fontRenderer, selEntry.name + ": " + selEntry.description, listW - 8),
                    listX + 4, fy + 3, 0xEEEEEE);
                std::string st = (es ? "Autor: " : "Author: ") + selEntry.author +
                    (selEntry.hasConfig ? (es ? "  |  [Y] Opciones" : "  |  [Y] Options") : "");
                drawString(fontRenderer, fitText(fontRenderer, st, listW - 8), listX + 4, fy + 13, 0x88DDFF);
            }
        }
    }

    // 5. Controls / Java buttons
    if (legacy)
    {
        const bool hasCfg = (selectedIndex >= 0 && selectedIndex < static_cast<int_t>(filteredEntries.size()) &&
                             filteredEntries[selectedIndex].hasConfig);
#if PLATFORM_XBOX
        if (hasCfg)
        {
            const std::string btns[] = {"A", "X", "Y", "White", "B"};
            const std::string acts[] = {
                es ? "Instalar" : "Toggle",
                es ? "Buscar" : "Search",
                es ? "Opciones" : "Opts",
                es ? "Filtro" : "Filter",
                es ? "Volver" : "Back"
            };
            drawControlHintRow(mc, width, hintY, btns, acts, 5);
        }
        else
        {
            const std::string btns[] = {"D-Pad", "A", "X", "White", "B"};
            const std::string acts[] = {
                es ? "Mover" : "Move",
                es ? "Instalar" : "Toggle",
                es ? "Buscar" : "Search",
                es ? "Filtro" : "Filter",
                es ? "Volver" : "Back"
            };
            drawControlHintRow(mc, width, hintY, btns, acts, 5);
        }
#endif
    }
    else
    {
        GuiScreen::drawScreen(mouseX, mouseY, partialTick);
    }
}
