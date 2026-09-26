#include "LegacyButtonPrompt.h"

#include "net/minecraft/src/FontRenderer.h"
#include "net/minecraft/src/RenderEngine.h"
#include "net/minecraft/src/Tessellator.h"
#include "platform/RenderAPI.h"

#include <algorithm>
#include <cctype>

namespace
{
static const Ps2ButtonPromptInfo s_iconInfos[] = {
    // Cross: rect=(2, 2, 60, 60)
    { 2.0f / 256.0f, 2.0f / 256.0f, 62.0f / 256.0f, 62.0f / 256.0f, 60, 60 },
    // Circle: rect=(66, 2, 60, 60)
    { 66.0f / 256.0f, 2.0f / 256.0f, 126.0f / 256.0f, 62.0f / 256.0f, 60, 60 },
    // Square: rect=(130, 2, 60, 60)
    { 130.0f / 256.0f, 2.0f / 256.0f, 190.0f / 256.0f, 62.0f / 256.0f, 60, 60 },
    // Triangle: rect=(194, 2, 60, 60)
    { 194.0f / 256.0f, 2.0f / 256.0f, 254.0f / 256.0f, 62.0f / 256.0f, 60, 60 },
    // DPad: rect=(2, 66, 60, 60)
    { 2.0f / 256.0f, 66.0f / 256.0f, 62.0f / 256.0f, 126.0f / 256.0f, 60, 60 },
    // L1: rect=(66, 76, 60, 40)
    { 66.0f / 256.0f, 76.0f / 256.0f, 126.0f / 256.0f, 116.0f / 256.0f, 60, 40 },
    // R1: rect=(130, 76, 60, 40)
    { 130.0f / 256.0f, 76.0f / 256.0f, 190.0f / 256.0f, 116.0f / 256.0f, 60, 40 },
    // L2: rect=(194, 74, 60, 43)
    { 194.0f / 256.0f, 74.0f / 256.0f, 254.0f / 256.0f, 117.0f / 256.0f, 60, 43 },
    // R2: rect=(2, 138, 60, 43)
    { 2.0f / 256.0f, 138.0f / 256.0f, 62.0f / 256.0f, 181.0f / 256.0f, 60, 43 },
    // L3: rect=(66, 130, 60, 60)
    { 66.0f / 256.0f, 130.0f / 256.0f, 126.0f / 256.0f, 190.0f / 256.0f, 60, 60 },
    // R3: rect=(130, 130, 60, 60)
    { 130.0f / 256.0f, 130.0f / 256.0f, 190.0f / 256.0f, 190.0f / 256.0f, 60, 60 },
    // Select: rect=(194, 138, 60, 44)
    { 194.0f / 256.0f, 138.0f / 256.0f, 254.0f / 256.0f, 182.0f / 256.0f, 60, 44 },
    // Start: rect=(2, 195, 60, 58)
    { 2.0f / 256.0f, 195.0f / 256.0f, 62.0f / 256.0f, 253.0f / 256.0f, 60, 58 }
};

#if PLATFORM_XBOX
#include "XboxButtonAtlasData.h"
#endif

static const Ps2ButtonPromptInfo s_defaultInfo = { 0.0f, 0.0f, 1.0f, 1.0f, 60, 60 };
} // namespace

namespace LegacyButtonPrompt
{

Ps2ButtonIcon iconFromName(const std::string &name)
{
    if (name.empty())
        return Ps2ButtonIcon::None;

#if PLATFORM_XBOX
    // Names from LegacyControlPromptBackend_XBOX / xboxPadKeyName.
    if (name == "A" || name == "[A]")
        return Ps2ButtonIcon::Cross;
    if (name == "B" || name == "[B]")
        return Ps2ButtonIcon::Circle;
    if (name == "X" || name == "[X]")
        return Ps2ButtonIcon::Square;
    if (name == "Y" || name == "[Y]")
        return Ps2ButtonIcon::Triangle;
    if (name == "White")
        return Ps2ButtonIcon::L1;
    if (name == "Black")
        return Ps2ButtonIcon::R1;
    if (name == "LT")
        return Ps2ButtonIcon::L2;
    if (name == "RT")
        return Ps2ButtonIcon::R2;
    if (name == "LS" || name == "Left Stick")
        return Ps2ButtonIcon::L3;
    if (name == "RS" || name == "Right Stick")
        return Ps2ButtonIcon::R3;
    if (name == "Back")
        return Ps2ButtonIcon::Select;
    if (name == "Start")
        return Ps2ButtonIcon::Start;
    if (name == "D-Pad Up")
        return Ps2ButtonIcon::DPadUp;
    if (name == "D-Pad Down")
        return Ps2ButtonIcon::DPadDown;
    if (name == "D-Pad Left")
        return Ps2ButtonIcon::DPadLeft;
    if (name == "D-Pad Right")
        return Ps2ButtonIcon::DPadRight;
    if (name.find("D-Pad") != std::string::npos || name.find("DPad") != std::string::npos)
        return Ps2ButtonIcon::DPad;
    return Ps2ButtonIcon::None;
#endif

    if (name == "Cross" || name == "X" || name == "[X]")
        return Ps2ButtonIcon::Cross;
    if (name == "Circle" || name == "O" || name == "[O]")
        return Ps2ButtonIcon::Circle;
    if (name == "Square" || name == "[ ]" || name == "[]")
        return Ps2ButtonIcon::Square;
    if (name == "Triangle" || name == "/\\" || name == "[/\\ ]" || name == "[/\\]")
        return Ps2ButtonIcon::Triangle;
    if (name.find("D-Pad") != std::string::npos || name.find("DPad") != std::string::npos || name == "Directional")
        return Ps2ButtonIcon::DPad;
    if (name == "L1" || name == "[L1]")
        return Ps2ButtonIcon::L1;
    if (name == "R1" || name == "[R1]")
        return Ps2ButtonIcon::R1;
    if (name == "L2" || name == "[L2]")
        return Ps2ButtonIcon::L2;
    if (name == "R2" || name == "[R2]")
        return Ps2ButtonIcon::R2;
    if (name == "L3" || name == "[L3]")
        return Ps2ButtonIcon::L3;
    if (name == "R3" || name == "[R3]")
        return Ps2ButtonIcon::R3;
    if (name == "Select" || name == "[Select]")
        return Ps2ButtonIcon::Select;
    if (name == "Start" || name == "[Start]")
        return Ps2ButtonIcon::Start;

    return Ps2ButtonIcon::None;
}

const Ps2ButtonPromptInfo &getIconInfo(Ps2ButtonIcon icon)
{
    int idx = static_cast<int>(icon);
#if PLATFORM_XBOX
    if (idx >= 0 && idx < static_cast<int>(sizeof(s_xboxIconInfos) / sizeof(s_xboxIconInfos[0])))
        return s_xboxIconInfos[idx];
#else
    if (idx >= 0 && idx < static_cast<int>(sizeof(s_iconInfos) / sizeof(s_iconInfos[0])))
        return s_iconInfos[idx];
#endif
    return s_defaultInfo;
}

int getIconDisplayWidth(Ps2ButtonIcon icon, int height)
{
    if (icon == Ps2ButtonIcon::None)
        return 0;
    const Ps2ButtonPromptInfo &info = getIconInfo(icon);
    return (height * info.pixelW + (info.pixelH / 2)) / info.pixelH;
}

int getAtlasTexture(RenderEngine *renderEngine)
{
    if (renderEngine == nullptr)
        return -1;
#if PLATFORM_XBOX
    return renderEngine->getTexture("/gui/buttons_xbox.png");
#else
    return renderEngine->getTexture("/gui/buttons_ps2.png");
#endif
}

void drawIcon(RenderEngine *renderEngine, Ps2ButtonIcon icon, int x, int y, int height)
{
    if (renderEngine == nullptr || icon == Ps2ButtonIcon::None)
        return;

    int texId = getAtlasTexture(renderEngine);
    if (texId < 0)
        return;

    renderEngine->bindTexture(texId);

    const Ps2ButtonPromptInfo &info = getIconInfo(icon);
    int w = getIconDisplayWidth(icon, height);

    renderEnable(RenderCapability::Blend);
    renderBlendFunc(RenderBlendFactor::SrcAlpha, RenderBlendFactor::OneMinusSrcAlpha);
    renderEnable(RenderCapability::Texture2D);
    renderColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    Tessellator *tess = &Tessellator::instance;
    tess->startDrawingQuads();
    tess->addVertexWithUV(x + 0, y + height, 0.0f, info.u0, info.v1);
    tess->addVertexWithUV(x + w, y + height, 0.0f, info.u1, info.v1);
    tess->addVertexWithUV(x + w, y + 0,      0.0f, info.u1, info.v0);
    tess->addVertexWithUV(x + 0, y + 0,      0.0f, info.u0, info.v0);
    tess->draw();

    renderDisable(RenderCapability::Blend);
}

int drawPrompt(RenderEngine *renderEngine, FontRenderer *font, Ps2ButtonIcon icon,
               const std::string &label, int x, int y, int height, int textColor)
{
    if (icon == Ps2ButtonIcon::None)
    {
        if (font != nullptr && !label.empty())
        {
            font->drawStringWithShadow(label, x, y, textColor);
            return font->getStringWidth(label);
        }
        return 0;
    }

    int w = getIconDisplayWidth(icon, height);
    drawIcon(renderEngine, icon, x, y - 1, height);

    if (font != nullptr && !label.empty())
    {
        font->drawStringWithShadow(label, x + w + 3, y, textColor);
        return w + 3 + font->getStringWidth(label);
    }
    return w;
}

int drawTwoButtonPrompt(RenderEngine *renderEngine, FontRenderer *font,
                        Ps2ButtonIcon icon1, Ps2ButtonIcon icon2,
                        const std::string &label, int x, int y, int height, int textColor)
{
    int w1 = getIconDisplayWidth(icon1, height);
    int w2 = getIconDisplayWidth(icon2, height);

    drawIcon(renderEngine, icon1, x, y - 1, height);
    drawIcon(renderEngine, icon2, x + w1 + 2, y - 1, height);

    int textX = x + w1 + 2 + w2 + 3;
    if (font != nullptr && !label.empty())
    {
        font->drawStringWithShadow(label, textX, y, textColor);
        return w1 + 2 + w2 + 3 + font->getStringWidth(label);
    }
    return w1 + 2 + w2;
}

int getPromptWidth(FontRenderer *font, Ps2ButtonIcon icon, const std::string &label, int height)
{
    int w = getIconDisplayWidth(icon, height);
    if (font != nullptr && !label.empty())
        return w + 3 + font->getStringWidth(label);
    return w;
}

int getTwoButtonPromptWidth(FontRenderer *font, Ps2ButtonIcon icon1, Ps2ButtonIcon icon2,
                            const std::string &label, int height)
{
    int w1 = getIconDisplayWidth(icon1, height);
    int w2 = getIconDisplayWidth(icon2, height);
    int w = w1 + 2 + w2;
    if (font != nullptr && !label.empty())
        return w + 3 + font->getStringWidth(label);
    return w;
}

} // namespace LegacyButtonPrompt
