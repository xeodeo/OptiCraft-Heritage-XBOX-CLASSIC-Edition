#pragma once

#include "net/minecraft/src/GuiScreen.h"
#include <string>

class GuiMinimapMenu : public GuiScreen
{
public:
    explicit GuiMinimapMenu(GuiScreen *parent = nullptr, int playerIndex = 0);
    ~GuiMinimapMenu() override = default;

    void initGui() override;
    void actionPerformed(GuiButton *button) override;
    void drawScreen(int_t mouseX, int_t mouseY, float_t partialTick) override;
    void handleSpecializedMenuInput() override;
    bool usesSpecializedMenuNavigation() const override;

protected:
    void keyTyped(char_t c, int_t key) override;

private:
    bool isLegacyMode() const;
    int_t getItemCount() const;
    std::string getItemLabel(int_t index) const;
    std::string getItemValue(int_t index) const;
    void activateItem(int_t index, int_t dir = 1);
    void refreshJavaButtons();
    void closeScreen();

    GuiScreen *m_parent;
    int m_playerIndex;
    int m_selectedIndex;
    int m_scrollOffset;
    bool m_openLatch;
};
