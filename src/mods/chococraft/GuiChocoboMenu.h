#pragma once

#include "net/minecraft/src/GuiScreen.h"
#include <string>

class EntityChocobo;
class EntityPlayer;
class GuiTextField;

class GuiChocoboMenu : public GuiScreen
{
public:
    GuiChocoboMenu(EntityChocobo *chocobo, EntityPlayer *player, int playerIndex = 0);
    ~GuiChocoboMenu() override;

    void initGui() override;
    void onGuiClosed() override;
    void updateScreen() override;
    void actionPerformed(GuiButton *button) override;
    void drawScreen(int_t mouseX, int_t mouseY, float_t partialTick) override;
    void handleSpecializedMenuInput() override;
    bool usesSpecializedMenuNavigation() const override;

protected:
    void keyTyped(char_t c, int_t key) override;

private:
    bool isLegacyMode() const;
    void drawLegacyScreen(float_t partialTick);
    void activateRow(int_t row);
    void beginRename();
    void commitRename();
    void cancelRename();
    void openSaddlebags();
    void refreshJavaButtons();
    void closeScreen();

    EntityChocobo *m_chocobo;
    EntityPlayer *m_player;
    int m_playerIndex;
    int m_selectedIndex;
    bool m_renaming;
    bool m_openLatch;
    GuiTextField *m_nameField;
};
