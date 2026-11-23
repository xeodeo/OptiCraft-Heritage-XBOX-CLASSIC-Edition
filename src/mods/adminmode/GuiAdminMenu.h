#pragma once

#include "net/minecraft/src/GuiScreen.h"
#include <string>

// Admin mode menu: one row per Nodus module (switch + its setting), Java and
// Legacy looks, full controller: D-Pad move, A switch, Left/Right setting,
// LB/RB page, X all off, B back. Player 2 gets its own copy in split screen.
class GuiAdminMenu : public GuiScreen
{
public:
    GuiAdminMenu(GuiScreen *parent, int playerIndex);
    ~GuiAdminMenu() override = default;

    void initGui() override;
    void actionPerformed(GuiButton *button) override;
    void drawScreen(int_t mouseX, int_t mouseY, float_t partialTick) override;
    void handleSpecializedMenuInput() override;
    bool usesSpecializedMenuNavigation() const override;
    bool allowsPlatformPointerInput() const override { return true; }

protected:
    void keyTyped(char_t c, int_t key) override;

private:
    bool isLegacyMode() const;
    int_t itemCount() const;
    std::string rowLabel(int_t idx, bool selected) const;
    std::string statusLine(int_t &color) const;
    void moveSelection(int_t delta);
    void toggleItem(int_t idx);
    void changeOption(int_t idx, int_t dir);
    void allOff();
    void refreshJavaButtons();
    void closeScreen();
    void click(float pitch);

    GuiScreen *m_parent;
    int m_playerIndex;
    int_t m_selectedIndex;
    int_t m_scrollOffset;
    int_t m_javaRows;
    bool m_openLatch;
};
