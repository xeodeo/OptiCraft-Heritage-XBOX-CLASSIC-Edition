#pragma once

#include "net/minecraft/src/GuiScreen.h"
#include <string>

class Minecraft;
class GuiTextField;

class GuiWaypointManager : public GuiScreen
{
public:
    explicit GuiWaypointManager(int playerIndex = 0);
    ~GuiWaypointManager() override;

    void initGui() override;
    void onGuiClosed() override;
    void drawScreen(int_t mouseX, int_t mouseY, float_t partialTick) override;
    void updateScreen() override;
    void handleSpecializedMenuInput() override;
    bool usesSpecializedMenuNavigation() const override;

protected:
    void keyTyped(char_t c, int_t key) override;

private:
    enum class EditMode
    {
        None,
        CreateNew,
        RenameExisting
    };

    bool isLegacyMode() const;
    void drawLegacyScreen(float_t partialTick);
    void beginCreateWaypoint();
    void beginRenameWaypoint();
    void commitTextEdit();
    void cancelTextEdit();
    void closeScreen();

    int m_playerIndex;
    int m_selectedIndex;
    int m_scrollOffset;
    int m_maxVisibleRows;
    EditMode m_editMode;
    GuiTextField *m_nameField;

    bool m_triangleLatch;
    bool m_downLatch;
    bool m_actionLatch;
    bool m_squareLatch;
    bool m_circleLatch;
    bool m_openLatch;

    unsigned short m_prevHeld;
    unsigned char m_prevLeftY;
};
