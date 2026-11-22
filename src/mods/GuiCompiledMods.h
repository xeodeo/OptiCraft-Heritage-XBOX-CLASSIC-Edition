#pragma once

#include "GuiScreen.h"
#include "ModManager.h"
#include <string>
#include <vector>

class GuiButton;
class GuiTextField;

// Store-style Compiled Mods Catalog & Search Screen (1b + 1c):
// - Lists all built-in C++ mods (+ optional local mods/catalog.txt)
// - Live search bar (Xbox VirtualKeyboard with X)
// - Filter tabs: All / Active / Available (White/Black)
// - Procedural pixel-art mod icons + description + 1-click Install/Activate (A)
// - Full Java UI & Legacy UI + 480p / 720p + full Xbox controller support.
class GuiCompiledMods : public GuiScreen
{
public:
    explicit GuiCompiledMods(GuiScreen *parent);
    ~GuiCompiledMods() override;

    void initGui() override;
    void onGuiClosed() override;
    void updateScreen() override;
    void actionPerformed(GuiButton *button) override;
    void drawScreen(int_t mouseX, int_t mouseY, float_t partialTick) override;
    void mouseClicked(int_t x, int_t y, int_t button) override;
    void keyTyped(char_t c, int_t key) override;
    void handleSpecializedMenuInput() override;
    bool usesSpecializedMenuNavigation() const override;
    bool allowsPlatformPointerInput() const override { return true; }

    static void drawModIcon(Gui *gui, const std::string &modId, int_t x, int_t y, int_t size);

private:
    enum class FilterTab : int
    {
        All = 0,
        Active = 1,
        Available = 2
    };

    bool isLegacyMode() const;
    void rebuildFilteredList();
    void toggleSelectedMod();
    void openSelectedModConfig();
    void beginSearch();
    void endSearch();
    void cycleFilter(int_t dir);
    void moveSelection(int_t delta);
    void clampSelection(int_t visibleRows);
    void refreshActionButtons();

    GuiScreen *parentScreen;
    std::vector<CompiledModInfo> allEntries;
    std::vector<CompiledModInfo> filteredEntries;
    FilterTab filterTab = FilterTab::All;
    int_t selectedIndex = 0;
    int_t scrollOffset = 0;
    bool searching = false;
    bool openLatch = true;
    GuiTextField *searchField = nullptr;
    GuiButton *installBtn = nullptr;
    GuiButton *configBtn = nullptr;
    GuiButton *searchBtn = nullptr;
    GuiButton *filterBtn = nullptr;
};
