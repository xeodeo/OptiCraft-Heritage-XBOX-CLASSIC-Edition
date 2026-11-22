#include "LegacyMenuWidgets.h"

#include <algorithm>

#include "LegacyGuiButtonStyle.h"
#include "LegacyGuiSprites.h"
#include "LegacyMenuHints.h"
#include "LegacyOptionText.h"
#include "LegacyOptionsLayout.h"
#include "LegacyOptionsPanel.h"
#include "LegacyPanorama.h"
#include "LegacyPauseStyle.h"
#include "LegacySceneState.h"
#include "LegacyUiTexture.h"
#include "LegacyUiTheme.h"
#include "net/minecraft/src/ControlIcon.h"
#include "net/minecraft/src/FontRenderer.h"
#include "net/minecraft/src/Gui.h"
#include "net/minecraft/src/HudSafeArea.h"
#include "net/minecraft/src/Minecraft.h"
#include "net/minecraft/src/RenderEngine.h"
#include "platform/RenderAPI.h"

namespace
{
// Gui keeps drawGradientRect protected; the helpers share one painter.
class LegacyMenuPainter : public Gui
{
public:
    void gradient(int_t x1, int_t y1, int_t x2, int_t y2, int_t top, int_t bottom)
    {
        drawGradientRect(x1, y1, x2, y2, top, bottom);
    }
};

LegacyMenuPainter &painter()
{
    static LegacyMenuPainter instance;
    return instance;
}

// Same optional sprites LegacyOptionCheckbox uses (procedural fallback below).
LegacyUiTexture g_menuTickBox("/legacy/tickbox.png");
LegacyUiTexture g_menuTickBoxHovered("/legacy/tickbox_hovered.png");
LegacyUiTexture g_menuTick("/legacy/tick.png");
}

int_t legacyMenuRowHeight(int_t screenHeight)
{
    if (screenHeight >= 330)
        return 18;
    if (screenHeight >= 220)
        return 16;
    return 14;
}

LegacyMenuLayout legacyMenuLayout(Minecraft *mc, int_t screenWidth, int_t screenHeight,
    int_t wantPanelW, int_t rowCount, int_t headerH, int_t footerH, int_t rowH)
{
    LegacyMenuLayout l{};
    const int_t sw = std::max<int_t>(1, screenWidth);
    const int_t sh = std::max<int_t>(1, screenHeight);
    l.split = (mc != nullptr && mc->isSplitScreenActive());
    l.hintY = legacyHintRowY(sh, l.split);

    const HudSafeInsets safe = computeHudSafeInsets(mc, sw, sh, true);
    const int_t top = std::max<int_t>(l.split ? 2 : 4, safe.top);
    // The hint icons are drawn 2 px above hintY; keep a small gap over them.
    const int_t bottom = l.hintY - (l.split ? 4 : 6);

    l.rowH = rowH > 0 ? rowH : legacyMenuRowHeight(sh);
    l.rowGap = (sh >= 330) ? 3 : 2;

    const int_t padBottom = l.split ? 4 : 6;
    const int_t fixedH = headerH + footerH + padBottom;
    const int_t room = std::max<int_t>(l.rowH, bottom - top - fixedH);
    const int_t fit = std::max<int_t>(1, (room + l.rowGap) / (l.rowH + l.rowGap));
    l.visibleRows = std::max<int_t>(1, std::min<int_t>(std::max<int_t>(1, rowCount), fit));

    const int_t rowsH = l.visibleRows * l.rowH + (l.visibleRows - 1) * l.rowGap;
    l.panelH = fixedH + rowsH;
    const int_t maxW = std::max<int_t>(120, sw - safe.left - safe.right - 8);
    l.panelW = std::min<int_t>(wantPanelW, maxW);
    l.panelX = (sw - l.panelW) / 2;
    l.panelY = top + std::max<int_t>(0, (bottom - top - l.panelH) / 2);

    l.titleY = l.panelY + (l.split ? 4 : 6);
    l.contentX = l.panelX + 8;
    l.contentW = l.panelW - 16;
    l.firstRowY = l.panelY + headerH;
    l.footerY = l.firstRowY + rowsH + 3;
    return l;
}

int_t legacyMenuKeepVisible(int_t selected, int_t count, int_t visibleRows, int_t scrollOffset)
{
    if (visibleRows < 1)
        visibleRows = 1;
    if (selected < scrollOffset)
        scrollOffset = selected;
    if (selected >= scrollOffset + visibleRows)
        scrollOffset = selected - visibleRows + 1;
    return std::max<int_t>(0, std::min<int_t>(scrollOffset, std::max<int_t>(0, count - visibleRows)));
}

void legacyDrawMenuBackdrop(Minecraft *mc, int_t screenWidth, int_t screenHeight, float_t partialTick)
{
    const bool inWorld = (mc != nullptr && mc->theWorld != nullptr);
    if (inWorld)
    {
        // Exactly the pause menu overlay: the world stays visible behind the panel.
        painter().gradient(0, 0, screenWidth, screenHeight,
            legacyPauseOverlayTopColor(), legacyPauseOverlayBottomColor());
        return;
    }
    if (mc != nullptr && legacyDrawPanorama(mc, screenWidth, screenHeight, legacyScenePanoramaTimer(), partialTick, 0.0f))
    {
        // Same light shade the Options screens put over the panorama.
        painter().gradient(0, 0, screenWidth, screenHeight,
            static_cast<int_t>(0x14000000u), static_cast<int_t>(0x3c000000u));
        return;
    }
    painter().gradient(0, 0, screenWidth, screenHeight,
        static_cast<int_t>(0xC0101010u), static_cast<int_t>(0xD0101010u));
}

void legacyDrawMenuPanel(int_t x, int_t y, int_t w, int_t h)
{
    LegacyOptionsLayout layout{};
    layout.panelX = x;
    layout.panelY = y;
    layout.panelWidth = w;
    layout.panelHeight = h;
    LegacyOptionsPanel panel;
    panel.draw(layout);
}

int_t legacyPanelTextColor()
{
    return static_cast<int_t>(0xff404040u);
}

void legacyDrawPanelTitle(FontRenderer *font, const std::string &text, int_t centerX, int_t y)
{
    if (font == nullptr)
        return;
    font->drawString(text, centerX - font->getStringWidth(text) / 2, y, legacyPanelTextColor());
}

void legacyDrawPanelText(FontRenderer *font, const std::string &text, int_t x, int_t y, int_t color)
{
    if (font == nullptr)
        return;
    font->drawString(text, x, y, color != 0 ? color : legacyPanelTextColor());
}

int_t legacyDrawPanelTextWrapped(FontRenderer *font, const std::string &text, int_t centerX, int_t y,
    int_t maxWidth, int_t color, int_t maxLines)
{
    if (font == nullptr || maxLines <= 0)
        return 0;
    std::string rem = text;
    int_t drawn = 0;
    while (!rem.empty() && drawn < maxLines)
    {
        std::string line;
        if (drawn == maxLines - 1 || font->getStringWidth(rem) <= maxWidth)
            line = legacyFitText(font, rem, maxWidth);
        else
        {
            line = font->trimStringToWidth(rem, maxWidth);
            const size_t sp = line.rfind(' ');
            if (line.size() < rem.size() && sp != std::string::npos && sp > 0)
                line = line.substr(0, sp);
        }
        if (line.empty())
            break;
        font->drawString(line, centerX - font->getStringWidth(line) / 2, y + drawn * 10,
            color != 0 ? color : legacyPanelTextColor());
        ++drawn;
        if (line.size() >= rem.size() || line.compare(line.size() >= 3 ? line.size() - 3 : 0, 3, "...") == 0)
            break;
        rem.erase(0, line.size());
        while (!rem.empty() && rem.front() == ' ')
            rem.erase(rem.begin());
    }
    return drawn;
}

int_t legacyDrawMenuButtonFrame(Minecraft *mc, int_t x, int_t y, int_t w, int_t h, bool selected, bool enabled)
{
    const LegacyGuiButtonVisual visual = legacyGuiButtonVisual(enabled, selected);
    if (mc != nullptr && mc->renderEngine != nullptr)
    {
        renderBindTexture(mc->renderEngine->getTexture("/gui/gui.png"));
        renderColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        legacyDrawVanillaButtonBase(x, y, w, h, visual.vanillaState, 0.0f);
    }
    if (visual.hovered)
    {
        painter().gradient(x + 1, y + 1, x + w - 1, y + h - 1, visual.hoverFillTop, visual.hoverFillBottom);
        Gui::drawRect(x + 2, y + 2, x + w - 2, y + 3, visual.hoverHighlightColor);
        Gui::drawRect(x + 2, y + h - 3, x + w - 2, y + h - 2, visual.hoverShadowColor);
    }
    return visual.textColor;
}

void legacyDrawMenuButton(Minecraft *mc, int_t x, int_t y, int_t w, int_t h,
    const std::string &label, bool selected, bool enabled)
{
    const int_t color = legacyDrawMenuButtonFrame(mc, x, y, w, h, selected, enabled);
    if (mc == nullptr || mc->fontRenderer == nullptr)
        return;
    legacyDrawCenteredOptionText(mc->fontRenderer, legacyFitText(mc->fontRenderer, label, w - 8),
        x + w / 2, legacyGuiButtonTextY(y, h), color);
}

void legacyDrawMenuValueButton(Minecraft *mc, int_t x, int_t y, int_t w, int_t h,
    const std::string &label, const std::string &value, bool selected, bool enabled)
{
    const int_t color = legacyDrawMenuButtonFrame(mc, x, y, w, h, selected, enabled);
    if (mc == nullptr || mc->fontRenderer == nullptr)
        return;
    FontRenderer *font = mc->fontRenderer;
    const int_t textY = legacyGuiButtonTextY(y, h);
    const int_t valueW = font->getStringWidth(value);
    legacyDrawOptionText(font, value, x + w - 7 - valueW, textY, color);
    legacyDrawOptionText(font, legacyFitText(font, label, w - valueW - 20), x + 7, textY, color);
}

void legacyDrawTickBox(Minecraft *mc, int_t x, int_t y, bool checked, bool hovered)
{
    const LegacyUiTheme &theme = legacyUiTheme();
    RenderEngine *engine = (mc != nullptr) ? mc->renderEngine : nullptr;
    const int_t sprite = theme.checkboxTextureSize;
    const int_t boxSize = theme.checkboxSize;
    const int_t boxX = x + (sprite - boxSize) / 2;
    const int_t boxY = y + (sprite - boxSize) / 2;
    const int_t boxTexture = engine != nullptr ? (hovered ? g_menuTickBoxHovered : g_menuTickBox).resolve(engine) : -1;
    if (boxTexture >= 0)
        legacyDrawUiTexture(boxTexture, x, y, sprite, sprite, 0.0f);
    else
    {
        Gui::drawRect(boxX, boxY, boxX + boxSize, boxY + boxSize, theme.checkboxBorderColor);
        Gui::drawRect(boxX + 1, boxY + 1, boxX + boxSize - 1, boxY + boxSize - 1,
            hovered ? theme.checkboxHoverFillColor : theme.checkboxFillColor);
        Gui::drawRect(boxX + 2, boxY + 2, boxX + boxSize - 2, boxY + boxSize - 2, theme.checkboxInnerColor);
    }
    if (!checked)
        return;
    const int_t tickTexture = engine != nullptr ? g_menuTick.resolve(engine) : -1;
    if (tickTexture >= 0)
        legacyDrawUiTexture(tickTexture, x + theme.checkboxTickOffsetX, y + theme.checkboxTickOffsetY,
            theme.checkboxTickWidth, theme.checkboxTickHeight, 0.0f);
    else
    {
        const int_t tick = hovered ? theme.checkboxTickHoverColor : theme.checkboxTickColor;
        Gui::drawRect(boxX + 2, boxY + 4, boxX + 4, boxY + 6, tick);
        Gui::drawRect(boxX + 4, boxY + 5, boxX + 6, boxY + 8, tick);
        Gui::drawRect(boxX + 6, boxY + 2, boxX + 8, boxY + 7, tick);
    }
}

void legacyDrawMenuCheckbox(Minecraft *mc, int_t x, int_t y, int_t w, int_t h,
    const std::string &label, bool checked, bool selected, bool enabled)
{
    const LegacyUiTheme &theme = legacyUiTheme();
    const int_t sprite = theme.checkboxTextureSize;
    legacyDrawTickBox(mc, x, y + (h - sprite) / 2, checked, selected && enabled);
    if (mc == nullptr || mc->fontRenderer == nullptr)
        return;
    const int_t color = !enabled ? theme.disabledTextColor
        : (selected ? theme.selectedTextColor : theme.normalTextColor);
    legacyDrawOptionText(mc->fontRenderer, legacyFitText(mc->fontRenderer, label, w - sprite - 6),
        x + sprite + 4, y + (h - 8 + 1) / 2, color);
}

void legacyDrawInsetBox(int_t x, int_t y, int_t w, int_t h, bool dark)
{
    Gui::drawRect(x, y, x + w, y + h, dark ? static_cast<int_t>(0xff1e1e1eu) : static_cast<int_t>(0xff8b8b8bu));
    Gui::drawRect(x, y, x + w - 1, y + 1, static_cast<int_t>(0xff373737u));
    Gui::drawRect(x, y, x + 1, y + h - 1, static_cast<int_t>(0xff373737u));
    Gui::drawRect(x + 1, y + h - 1, x + w, y + h, static_cast<int_t>(0xffffffffu));
    Gui::drawRect(x + w - 1, y + 1, x + w, y + h, static_cast<int_t>(0xffffffffu));
}

void legacyDrawTextFieldBox(int_t x, int_t y, int_t w, int_t h, bool focused)
{
    const int_t edge = focused ? static_cast<int_t>(0xffffff55u) : static_cast<int_t>(0xffa0a0a0u);
    Gui::drawRect(x - 1, y - 1, x + w + 1, y + h + 1, edge);
    Gui::drawRect(x, y, x + w, y + h, static_cast<int_t>(0xff000000u));
}

void legacyDrawMenuScrollbar(int_t x, int_t y, int_t h, int_t scrollOffset, int_t visibleRows, int_t count)
{
    if (count <= visibleRows || h <= 4)
        return;
    Gui::drawRect(x, y, x + 4, y + h, static_cast<int_t>(0xff373737u));
    Gui::drawRect(x + 1, y + 1, x + 3, y + h - 1, static_cast<int_t>(0xff8b8b8bu));
    const int_t inner = h - 2;
    const int_t maxScroll = std::max<int_t>(1, count - visibleRows);
    const int_t thumbH = std::max<int_t>(6, inner * visibleRows / count);
    const int_t thumbY = y + 1 + (inner - thumbH) * scrollOffset / maxScroll;
    Gui::drawRect(x + 1, thumbY, x + 3, thumbY + thumbH, static_cast<int_t>(0xffeeeeeeu));
    Gui::drawRect(x + 2, thumbY + 1, x + 3, thumbY + thumbH, static_cast<int_t>(0xffb0b0b0u));
}

void legacyDrawMenuTab(Minecraft *mc, int_t x, int_t y, int_t w, int_t h, const std::string &label, bool active)
{
    legacyDrawMenuButton(mc, x, y, w, h, label, active, true);
}

std::string legacyFitText(FontRenderer *font, const std::string &text, int_t maxWidth)
{
    if (font == nullptr || maxWidth <= 8 || font->getStringWidth(text) <= maxWidth)
        return text;
    std::string out = text;
    while (!out.empty() && font->getStringWidth(out + "...") > maxWidth)
    {
        out.pop_back();
        // Never leave half of a UTF-8 character (accents in the Spanish text).
        while (!out.empty() && (static_cast<unsigned char>(out.back()) & 0xC0u) == 0x80u)
            out.pop_back();
        if (!out.empty() && (static_cast<unsigned char>(out.back()) & 0xC0u) == 0xC0u)
            out.pop_back();
    }
    return out + "...";
}

void legacyDrawInlineHints(Minecraft *mc, int_t centerX, int_t y,
    const std::string *buttons, const std::string *actions, int_t count)
{
    if (mc == nullptr || mc->fontRenderer == nullptr || count <= 0 || count > 6)
        return;
    FontRenderer *font = mc->fontRenderer;
    ControlIcon icons[6];
    std::string texts[6];
    int_t widths[6] = {};
    int_t total = 0;
    for (int_t i = 0; i < count; ++i)
    {
        icons[i] = controlIconTexture(mc, buttons[i]);
        texts[i] = icons[i].texture >= 0 ? actions[i] : ("[" + buttons[i] + "] " + actions[i]);
        widths[i] = font->getStringWidth(texts[i]) + (icons[i].texture >= 0 ? 15 : 0);
        total += widths[i];
    }
    total += (count - 1) * 8;
    int_t x = centerX - total / 2;
    for (int_t i = 0; i < count; ++i)
    {
        int_t textX = x;
        if (icons[i].texture >= 0)
        {
            drawControlIcon(mc, icons[i], x, y - 2);
            textX += 15;
        }
        font->drawString(texts[i], textX, y, legacyPanelTextColor());
        x += widths[i] + 8;
    }
}
