#include "LegacyGuiSprites.h"

#include "net/minecraft/src/Tessellator.h"

namespace
{
// Source rows of the widget art inside /gui/gui.png.
constexpr int_t BUTTON_SOURCE_WIDTH = 200;
constexpr int_t BUTTON_SOURCE_HEIGHT = 20;
constexpr int_t BUTTON_SOURCE_Y = 46;
constexpr int_t SLIDER_KNOB_SOURCE_Y = 66;
constexpr int_t SLIDER_KNOB_SOURCE_WIDTH = 4;
}

void legacyDrawGuiAtlasRect(int_t x, int_t y, int_t width, int_t height, int_t texX, int_t texY,
    int_t sourceWidth, int_t sourceHeight, float_t zLevel)
{
    constexpr float_t INV_ATLAS = 1.0f / 256.0f;
    const float_t u0 = static_cast<float_t>(texX) * INV_ATLAS;
    const float_t u1 = static_cast<float_t>(texX + sourceWidth) * INV_ATLAS;
    const float_t v0 = static_cast<float_t>(texY) * INV_ATLAS;
    const float_t v1 = static_cast<float_t>(texY + sourceHeight) * INV_ATLAS;
    Tessellator *tess = &Tessellator::instance;
    tess->startDrawingQuads();
    tess->addVertexWithUV(x, y + height, zLevel, u0, v1);
    tess->addVertexWithUV(x + width, y + height, zLevel, u1, v1);
    tess->addVertexWithUV(x + width, y, zLevel, u1, v0);
    tess->addVertexWithUV(x, y, zLevel, u0, v0);
    tess->draw();
}

void legacyDrawVanillaButtonBase(int_t x, int_t y, int_t width, int_t height, int_t state,
    float_t zLevel)
{
    const int_t texY = BUTTON_SOURCE_Y + state * BUTTON_SOURCE_HEIGHT;
    const int_t half = BUTTON_SOURCE_WIDTH / 2;
    if (width > BUTTON_SOURCE_WIDTH)
    {
        // Wider than the 200 px sprite (rows at 720p): both ends from the sprite and
        // its middle stretched, instead of reading past the sprite (a stripe in the
        // middle of every row, user 23-11).
        const int_t middle = width - 2 * half;
        legacyDrawGuiAtlasRect(x, y, half, height, 0, texY, half, BUTTON_SOURCE_HEIGHT, zLevel);
        legacyDrawGuiAtlasRect(x + half, y, middle, height, half - 50, texY, 100, BUTTON_SOURCE_HEIGHT, zLevel);
        legacyDrawGuiAtlasRect(x + half + middle, y, half, height, half, texY, half, BUTTON_SOURCE_HEIGHT, zLevel);
        return;
    }
    const int_t leftWidth = width / 2;
    const int_t rightWidth = width - leftWidth;
    legacyDrawGuiAtlasRect(x, y, leftWidth, height, 0, texY, leftWidth, BUTTON_SOURCE_HEIGHT, zLevel);
    legacyDrawGuiAtlasRect(x + leftWidth, y, rightWidth, height, BUTTON_SOURCE_WIDTH - rightWidth,
        texY, rightWidth, BUTTON_SOURCE_HEIGHT, zLevel);
}

void legacyDrawVanillaSliderKnob(int_t x, int_t y, int_t width, int_t height, float_t zLevel)
{
    const int_t leftWidth = width / 2;
    const int_t rightWidth = width - leftWidth;
    legacyDrawGuiAtlasRect(x, y, leftWidth, height, 0, SLIDER_KNOB_SOURCE_Y,
        SLIDER_KNOB_SOURCE_WIDTH, BUTTON_SOURCE_HEIGHT, zLevel);
    legacyDrawGuiAtlasRect(x + leftWidth, y, rightWidth, height, 196, SLIDER_KNOB_SOURCE_Y,
        SLIDER_KNOB_SOURCE_WIDTH, BUTTON_SOURCE_HEIGHT, zLevel);
}
