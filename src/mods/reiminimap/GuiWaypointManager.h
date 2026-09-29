#pragma once

#include "net/minecraft/src/GuiScreen.h"
#include <string>

class Minecraft;

class GuiWaypointManager : public GuiScreen
{
public:
    explicit GuiWaypointManager(int playerIndex = 0);
    ~GuiWaypointManager() override = default;

    void initGui() override;
    void drawScreen(int_t mouseX, int_t mouseY, float_t partialTick) override;
    void updateScreen() override;
    void handleSpecializedMenuInput() override;
    bool usesSpecializedMenuNavigation() const override { return true; }

protected:
    void keyTyped(char_t c, int_t key) override;

private:
    void closeScreen();

    int m_playerIndex;
    int m_selectedIndex;
    int m_scrollOffset;
    int m_maxVisibleRows;

    bool m_triangleLatch;
    bool m_downLatch;
    bool m_actionLatch;
    bool m_squareLatch;
    bool m_circleLatch;

    unsigned short m_prevHeld;
    unsigned char m_prevLeftY;
};
