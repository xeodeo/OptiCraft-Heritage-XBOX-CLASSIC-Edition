#pragma once

#include "java/Type.h"
#include <string>

class Minecraft;
class FontRenderer;

// Building blocks for the small Legacy (LCE) menus added with the mods, skins
// and server tools: the same grey bevelled panel, stone buttons with the yellow
// selected label, TickBox and hint row the Options / Create World screens use,
// so those menus stop looking like a separate (Bedrock-like) flat list.

// Panel + rows of one Legacy list menu, fitted between the top of the HUD safe
// area and the hint row, centred on the screen. Rows that do not fit scroll.
struct LegacyMenuLayout
{
    int_t panelX = 0;
    int_t panelY = 0;
    int_t panelW = 0;
    int_t panelH = 0;
    int_t titleY = 0;      // first header line (panel title)
    int_t contentX = 0;
    int_t contentW = 0;
    int_t firstRowY = 0;   // y of the first list row
    int_t rowH = 16;
    int_t rowGap = 2;
    int_t visibleRows = 1;
    int_t footerY = 0;     // first line under the rows (inside the panel)
    int_t hintY = 0;       // hint row baseline (outside the panel)
    bool split = false;

    int_t rowY(int_t visibleIndex) const { return firstRowY + visibleIndex * (rowH + rowGap); }
};

// headerH: room above the rows (title, subtitle...). footerH: room under them
// (description line...). rowH <= 0 picks the Legacy row height for the screen.
LegacyMenuLayout legacyMenuLayout(Minecraft *mc, int_t screenWidth, int_t screenHeight,
    int_t wantPanelW, int_t rowCount, int_t headerH, int_t footerH, int_t rowH = 0);

// Standard Legacy row height for this screen height (16 at 480p, 18 at 720p,
// 14 in split screen).
int_t legacyMenuRowHeight(int_t screenHeight);

// Scroll offset that keeps selected inside [scroll, scroll + visible).
int_t legacyMenuKeepVisible(int_t selected, int_t count, int_t visibleRows, int_t scrollOffset);

// Panorama in the menus, dimmed world (pause overlay) in game.
void legacyDrawMenuBackdrop(Minecraft *mc, int_t screenWidth, int_t screenHeight, float_t partialTick);

void legacyDrawMenuPanel(int_t x, int_t y, int_t w, int_t h);

// Dark grey panel text (LCE titles / labels sit on the light panel without a
// drop shadow, only the soft emboss).
void legacyDrawPanelTitle(FontRenderer *font, const std::string &text, int_t centerX, int_t y);
void legacyDrawPanelText(FontRenderer *font, const std::string &text, int_t x, int_t y, int_t color = 0);
int_t legacyPanelTextColor();
// Centred panel text wrapped on spaces over up to maxLines lines (10 px apart);
// returns the lines drawn.
int_t legacyDrawPanelTextWrapped(FontRenderer *font, const std::string &text, int_t centerX, int_t y,
    int_t maxWidth, int_t color, int_t maxLines);

// Stone /gui/gui.png button with the blue-grey selected fill. Returns the label
// colour (white, yellow when selected, grey when disabled).
int_t legacyDrawMenuButtonFrame(Minecraft *mc, int_t x, int_t y, int_t w, int_t h, bool selected, bool enabled);
void legacyDrawMenuButton(Minecraft *mc, int_t x, int_t y, int_t w, int_t h,
    const std::string &label, bool selected, bool enabled = true);
// Button with the label on the left and the value on the right ("Zoom   2x").
void legacyDrawMenuValueButton(Minecraft *mc, int_t x, int_t y, int_t w, int_t h,
    const std::string &label, const std::string &value, bool selected, bool enabled = true);

// LCE TickBox row on the panel: box on the left, dark label (yellow when selected).
void legacyDrawMenuCheckbox(Minecraft *mc, int_t x, int_t y, int_t w, int_t h,
    const std::string &label, bool checked, bool selected, bool enabled = true);
// Just the box (+ tick) at x,y (12x12 sprite area).
void legacyDrawTickBox(Minecraft *mc, int_t x, int_t y, bool checked, bool hovered);

// Recessed slot-style box (dark top-left / white bottom-right edges, the
// container look). Light = 0x8B8B8B slot grey; dark = the near-black list well
// of the Legacy flat-world / preset lists, for coloured text and previews.
void legacyDrawInsetBox(int_t x, int_t y, int_t w, int_t h, bool dark = false);
// Black text field look (Create World seed box); yellow edge when focused.
void legacyDrawTextFieldBox(int_t x, int_t y, int_t w, int_t h, bool focused);
// Thin scrollbar on the panel.
void legacyDrawMenuScrollbar(int_t x, int_t y, int_t h, int_t scrollOffset, int_t visibleRows, int_t count);

// Small LCE tab (stone button style) used for filter tabs in a header row.
void legacyDrawMenuTab(Minecraft *mc, int_t x, int_t y, int_t w, int_t h, const std::string &label, bool active);

// Button icons + dark labels on the panel itself, centred on centerX (for the
// extra buttons that do not fit in the 4-entry hint row at the bottom).
void legacyDrawInlineHints(Minecraft *mc, int_t centerX, int_t y,
    const std::string *buttons, const std::string *actions, int_t count);

std::string legacyFitText(FontRenderer *font, const std::string &text, int_t maxWidth);
