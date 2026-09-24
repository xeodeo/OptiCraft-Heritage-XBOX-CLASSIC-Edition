#pragma once

#include "java/Type.h"
#include <string>

class RenderEngine;
class FontRenderer;

enum class Ps2ButtonIcon : int
{
    None = -1,
    Cross = 0,
    Circle = 1,
    Square = 2,
    Triangle = 3,
    DPad = 4,
    L1 = 5,
    R1 = 6,
    L2 = 7,
    R2 = 8,
    L3 = 9,
    R3 = 10,
    Select = 11,
    Start = 12
};

struct Ps2ButtonPromptInfo
{
    float u0, v0, u1, v1;
    int pixelW, pixelH;
};

namespace LegacyButtonPrompt
{
    // Map text name (e.g. "Cross", "Triangle", "R2", "D-Pad", etc.) to icon enum
    Ps2ButtonIcon iconFromName(const std::string &name);

    // Get info (UVs and pixel dimensions) for an icon
    const Ps2ButtonPromptInfo &getIconInfo(Ps2ButtonIcon icon);

    // Get display width of an icon given target display height (default 11)
    int getIconDisplayWidth(Ps2ButtonIcon icon, int height = 11);

    // Get or load the button atlas texture ID
    int getAtlasTexture(RenderEngine *renderEngine);

    // Draw single icon at (x, y) with specified height
    void drawIcon(RenderEngine *renderEngine, Ps2ButtonIcon icon, int x, int y, int height = 11);

    // Draw an icon + text prompt: icon at (x, y - 1), text at (x + iconW + 3, y)
    // Returns total advance width
    int drawPrompt(RenderEngine *renderEngine, FontRenderer *font, Ps2ButtonIcon icon,
                   const std::string &label, int x, int y, int height = 11, int textColor = 0xffffff);

    // Draw two icons (e.g. [L1][R1]) + text prompt
    int drawTwoButtonPrompt(RenderEngine *renderEngine, FontRenderer *font,
                            Ps2ButtonIcon icon1, Ps2ButtonIcon icon2,
                            const std::string &label, int x, int y, int height = 11, int textColor = 0xffffff);

    // Calculate prompt width without drawing
    int getPromptWidth(FontRenderer *font, Ps2ButtonIcon icon, const std::string &label, int height = 11);

    int getTwoButtonPromptWidth(FontRenderer *font, Ps2ButtonIcon icon1, Ps2ButtonIcon icon2,
                                const std::string &label, int height = 11);
}
