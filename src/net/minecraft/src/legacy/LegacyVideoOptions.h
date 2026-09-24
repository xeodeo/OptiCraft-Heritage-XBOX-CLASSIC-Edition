#pragma once

#include "LegacyOptionsScreen.h"

class GuiButton;
class LegacyOptionCheckbox;
class LegacyOptionSlider;

class LegacyVideoOptions : public LegacyOptionsScreen
{
public:
    LegacyVideoOptions(GuiScreen *parent, GameSettings *settings,
        LegacyOptionsBackgroundMode backgroundMode = LegacyOptionsBackgroundMode::Panorama);
    void initGui() override;
    void drawScreen(int_t mouseX, int_t mouseY, float_t partialTick) override;

protected:
    void actionPerformed(GuiButton *button) override;

private:
    void syncCheckboxes();

    LegacyOptionCheckbox *graphicsCheckbox;
    LegacyOptionCheckbox *smoothLightingCheckbox;
    LegacyOptionCheckbox *viewBobbingCheckbox;
    LegacyOptionCheckbox *cloudsCheckbox;
    LegacyOptionCheckbox *fogCheckbox;
    // Wii only: the EFB->XFB deflicker filter; null elsewhere.
    LegacyOptionCheckbox *deflickerCheckbox;
    // Xbox only: 30 FPS cap and the coordinates HUD line; null elsewhere.
    LegacyOptionCheckbox *frameCapCheckbox;
    LegacyOptionCheckbox *coordinatesCheckbox;
};
