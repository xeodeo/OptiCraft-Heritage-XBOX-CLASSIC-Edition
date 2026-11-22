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
void setComboEnabled(bool enabled);
}
