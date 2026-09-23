#pragma once

#include "net/minecraft/src/GuiSelectWorld.h"
#include "LegacyOptionsLayout.h"
#include "LegacyOptionsPanel.h"

class LegacyPlayGameScreen : public GuiSelectWorld
{
public:
    explicit LegacyPlayGameScreen(GuiScreen *parent);

    void initGui() override;
    void updateScreen() override;
    void drawScreen(int_t mouseX, int_t mouseY, float_t partialTick) override;
    void handleMouseInput() override;

protected:
    bool usesSpecializedMenuNavigation() const override { return true; }
    void actionPerformed(GuiButton *button) override;
    void keyTyped(char_t c, int_t key) override;

private:
    void rebuildButtons();
    void drawLegacyScene(float_t partialTick);
    void drawEntryIcons();
    void drawScrollIndicators();
    void drawMenuControlHints();
    void syncSelectedButton();
    void activateSelection();
    void selectControl(int_t index);
    void moveSelection(int_t direction);
    int_t selectedWorldIndex() const;
    void requestDeleteSelectedWorld();
    int_t maxVisibleWorlds() const;
    int_t maxPage() const;

    LegacyOptionsLayout layout;
    LegacyOptionsPanel panelRenderer;
    int_t page;
    int_t firstWorldIndex;
    int_t visibleWorldCount;
    int_t selectedControlIndex;
    int_t hoveredControlIndex;
    int_t tutorialMessageTicks;
    std::string tutorialMessage;
    int_t lastMouseX;
    int_t lastMouseY;
    bool panoramaAvailable;
};
