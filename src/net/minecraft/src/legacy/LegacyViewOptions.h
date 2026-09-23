#pragma once

#include "LegacyOptionsScreen.h"

class GuiButton;
class LegacyOptionCheckbox;

class LegacyViewOptions : public LegacyOptionsScreen
{
public:
    LegacyViewOptions(GuiScreen *parent, GameSettings *settings,
        LegacyOptionsBackgroundMode backgroundMode = LegacyOptionsBackgroundMode::Panorama);
    void initGui() override;
    void drawScreen(int_t mouseX, int_t mouseY, float_t partialTick) override;

protected:
    void actionPerformed(GuiButton *button) override;

private:
    LegacyOptionCheckbox *invertMouseCheckbox;
    LegacyOptionCheckbox *dolbyCheckbox;  // Xbox only
};
