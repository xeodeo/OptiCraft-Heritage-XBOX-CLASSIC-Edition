#pragma once

#include "net/minecraft/src/GuiScreen.h"

class GuiButton;

// Herobrine Classic (tkelly iLoveYou) Options Screen.
// Every event is toggleable; destructive or chest-stealing events default to OFF
// with a clear warning shown in the footer. Supports Java UI + Legacy UI + Xbox controller.
class GuiHerobrineMenu : public GuiScreen
{
public:
    explicit GuiHerobrineMenu(GuiScreen *parent);
    ~GuiHerobrineMenu() override = default;

    void initGui() override;
    void actionPerformed(GuiButton *button) override;
    void drawScreen(int_t mouseX, int_t mouseY, float_t partialTick) override;
    void keyTyped(char_t c, int_t key) override;
    void handleSpecializedMenuInput() override;
    bool usesSpecializedMenuNavigation() const override;
    bool allowsPlatformPointerInput() const override { return true; }

private:
    bool isLegacyMode() const;
    int_t getItemCount() const { return 7; }
    std::string getItemLabel(int_t idx) const;
    std::string getItemValue(int_t idx) const;
    std::string getItemDesc(int_t idx) const;
    bool isDestructiveItem(int_t idx) const;
    void toggleItem(int_t idx);
    void ensureSelectionVisible(int_t visibleRows);
    void refreshJavaButtons();
    void closeScreen();

    GuiScreen *m_parent;
    int_t m_selectedIndex;
    int_t m_scrollOffset;
    bool m_openLatch;
};
