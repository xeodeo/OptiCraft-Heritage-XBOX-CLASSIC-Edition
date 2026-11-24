#pragma once

// Render-side X-Ray state. The compiled X-Ray mod toggles it; terrain
// renderers read it to hide blocks other than ores and valuables.
namespace XRayMode
{
bool active();
void setActive(bool active);
void toggle();
void reset();
bool isVisibleBlock(int blockID);

bool comboEnabled();
// Admin mode CaveFinder (Nodus): draw only stone so caves stand out.
void setCaveMode(bool on);
bool caveMode();
void setComboEnabled(bool enabled);
}
