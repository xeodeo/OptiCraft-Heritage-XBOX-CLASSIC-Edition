#include "LegacyTipHud.h"

#include "LegacyOptionsPanel.h"
#include "LegacyUiTexture.h"

#include "net/minecraft/src/FontRenderer.h"
#include "net/minecraft/src/GameSettings.h"
#include "net/minecraft/src/HudSafeArea.h"
#include "net/minecraft/src/Minecraft.h"
#include "net/minecraft/src/RenderEngine.h"
#include "platform/RenderAPI.h"

#include <algorithm>
#include <cmath>
#include <deque>
#include <string>
#include <vector>

// Mirrors Legacy4J's in-game LegacyTip(title, tip) in its SD (console)
// metrics: a panel up to 130 px wide, text 7 px in on 8 px lines, height 13 +
// text. Legacy4J pairs SD with its smaller Mojangles 11 font; this port only
// has the regular 9 px font, so the text is drawn at 2/3 scale instead -- the
// same stand-in Legacy4J uses for its potion labels when that font is absent.
// Tucked into the top right corner the way Console Edition shows its tutorial
// tips, which pop in and out with no animation.
namespace
{
constexpr int_t TIP_MAX_WIDTH = 130;
constexpr int_t TIP_TEXT_INSET = 7;
constexpr int_t TIP_LINE_SPACING = 8;
constexpr int_t TIP_BASE_HEIGHT = 13;
constexpr float_t TIP_TEXT_SCALE = 0.75f;
constexpr int_t TIP_RIGHT_MARGIN = 16;
constexpr int_t TIP_TOP = 16;
constexpr int_t TIP_TEXT_COLOR = 0xffffff;
constexpr int_t TIP_DISPLAY_TICKS = 60;   // 3 s

// pointer_panel.png is Legacy4J's 24 x 24 tile scaled as a 12 x 12 nine-slice
// with a 4 px border.
constexpr int_t TIP_PANEL_SPRITE_SIZE = 12;
constexpr int_t TIP_PANEL_SPRITE_BORDER = 4;
LegacyUiTexture g_pointerPanel("/legacy/pointer_panel.png");

// Flat stand-in in the sprite's own colours for a pack without the texture.
const LegacyPanelColors TIP_FALLBACK_COLORS = {
    static_cast<int_t>(0xd12f4049u), // fill
    static_cast<int_t>(0xffebebebu), // border
    static_cast<int_t>(0xff323232u), // highlight
    static_cast<int_t>(0xff323232u), // shadow
    static_cast<int_t>(0x3d000000u), // drop shadow
};

struct Tip
{
    std::string text;
    std::vector<std::string> lines;
};

std::deque<Tip> s_queue;
// Ticks the current tip has been on screen; -1 while nothing is showing.
int_t s_ticks = -1;
LegacyOptionsPanel s_panel;

// Word wrap on the regular font. Done once per tip rather than through
// drawSplitString(), whose 9 px line pitch is not the toast's 12 px.
void wrapLines(FontRenderer *font, const std::string &text, int_t width, std::vector<std::string> &lines)
{
    lines.clear();
    std::string line;
    std::string word;
    const auto flush = [&]()
    {
        if (word.empty())
            return;
        const std::string candidate = line.empty() ? word : line + " " + word;
        if (!line.empty() && font->getStringWidth(candidate) > width)
        {
            lines.push_back(line);
            line = word;
        }
        else
        {
            line = candidate;
        }
        word.clear();
    };
    for (char c : text)
    {
        if (c == ' ')
            flush();
        else if (c == '\n')
        {
            flush();
            lines.push_back(line);
            line.clear();
        }
        else
            word += c;
    }
    flush();
    if (!line.empty() || lines.empty())
        lines.push_back(line);
}
}

void LegacyTipHud::show(const std::string &text)
{
    if (text.empty())
        return;
    Tip tip;
    tip.text = text;
    s_queue.push_back(tip);
}

void LegacyTipHud::clear()
{
    s_queue.clear();
    s_ticks = -1;
}

void LegacyTipHud::tick()
{
    if (s_queue.empty())
    {
        s_ticks = -1;
        return;
    }
    if (s_ticks < 0)
    {
        s_ticks = 0;
        return;
    }
    if (++s_ticks >= TIP_DISPLAY_TICKS)
    {
        s_queue.pop_front();
        s_ticks = -1;
    }
}

void LegacyTipHud::render(Minecraft *mc, int_t screenWidth, int_t screenHeight)
{
    (void)screenHeight;
    if (mc == nullptr || mc->gameSettings == nullptr || mc->fontRenderer == nullptr)
        return;
    if (mc->theWorld == nullptr)
    {
        clear();
        return;
    }
    if (s_queue.empty() || s_ticks < 0)
        return;
    if (!mc->gameSettings->legacyUI || mc->gameSettings->hideGUI || mc->currentScreen != nullptr)
        return;

    FontRenderer *font = mc->fontRenderer;
    Tip &tip = s_queue.front();
    const int_t maxWidth = std::min<int_t>(TIP_MAX_WIDTH, screenWidth);
    // Wrap in unscaled font units: the scaled text has 1 / TIP_TEXT_SCALE
    // times the panel's room.
    if (tip.lines.empty())
        wrapLines(font, tip.text, std::max<int_t>(1,
            static_cast<int_t>((maxWidth - TIP_TEXT_INSET * 2) / TIP_TEXT_SCALE)), tip.lines);

    int_t widestLine = 0;
    for (const std::string &line : tip.lines)
        widestLine = std::max<int_t>(widestLine, font->getStringWidth(line));

    const int_t textWidth = static_cast<int_t>(std::ceil(widestLine * TIP_TEXT_SCALE));
    const int_t width = std::min<int_t>(maxWidth, textWidth + TIP_TEXT_INSET * 2);
    const int_t textHeight = static_cast<int_t>(tip.lines.size()) * TIP_LINE_SPACING;
    const int_t height = TIP_BASE_HEIGHT + textHeight;
    const HudSafeInsets safe = computeHudSafeInsets(mc, screenWidth, screenHeight, true);
    const int_t x = std::max<int_t>(0, screenWidth - width - std::max<int_t>(TIP_RIGHT_MARGIN, safe.right));
    const int_t y = std::max<int_t>(TIP_TOP, safe.top);

    const int_t panelTexture = g_pointerPanel.resolve(mc->renderEngine);
    if (panelTexture >= 0)
    {
        legacyDrawUiTextureNineSlice(panelTexture, x, y, width, height,
            TIP_PANEL_SPRITE_SIZE, TIP_PANEL_SPRITE_SIZE, TIP_PANEL_SPRITE_BORDER, 0.0f);
    }
    else
    {
        LegacyOptionsLayout layout{};
        layout.panelX = x;
        layout.panelY = y;
        layout.panelWidth = width;
        layout.panelHeight = height;
        renderEnable(RenderCapability::Blend);
        renderBlendFunc(RenderBlendFactor::SrcAlpha, RenderBlendFactor::OneMinusSrcAlpha);
        s_panel.drawFrame(layout, TIP_FALLBACK_COLORS);
        renderDisable(RenderCapability::Blend);
    }

    renderPushMatrix();
    renderTranslate(static_cast<float_t>(x + TIP_TEXT_INSET), static_cast<float_t>(y + TIP_TEXT_INSET), 0.0f);
    renderScale(TIP_TEXT_SCALE, TIP_TEXT_SCALE, 1.0f);
    int_t lineY = 0;
    for (const std::string &line : tip.lines)
    {
        font->drawString(line, 0, lineY, TIP_TEXT_COLOR);
        lineY += static_cast<int_t>(TIP_LINE_SPACING / TIP_TEXT_SCALE);
    }
    renderPopMatrix();
}
