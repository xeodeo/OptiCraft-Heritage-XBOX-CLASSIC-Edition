#include "LegacyOptionsLayout.h"
#include "net/minecraft/src/Minecraft.h"

#include <algorithm>

#include "LegacyOptionMetrics.h"
#include "LegacyMenuHints.h"
#include "LegacySceneLayout.h"
#include "LegacyUiTheme.h"
#include "platform/PlatformTuning.h"

namespace
{
struct LegacyOptionsLayoutProfile
{
    int_t targetWidth;
    int_t minimumWidth;
    int_t contentPadding;
    int_t rowHeight;
    int_t rowSpacing;
    int_t topPadding;
    int_t bottomPadding;
    int_t footerGap;
    bool scaleRows;
};

constexpr int_t LEGACY_OPTIONS_FOOTER_GAP = 14;
constexpr int_t LEGACY_COMPACT_PANEL_WIDTH = 188;
constexpr int_t LEGACY_FORM_MIN_PANEL_WIDTH = 172;
constexpr int_t LEGACY_FORM_BASE_MIN_PANEL_WIDTH = 184;
constexpr int_t LEGACY_FORM_PANEL_REDUCTION = 12;
constexpr int_t LEGACY_FORM_SCREEN_INSET = 16;
constexpr int_t LEGACY_FORM_CONTENT_PADDING = 10;
constexpr int_t LEGACY_FORM_ROW_HEIGHT = 17;
constexpr int_t LEGACY_FORM_ROW_SPACING = 3;
constexpr int_t LEGACY_FORM_TOP_PADDING = 24;
constexpr int_t LEGACY_FORM_BOTTOM_PADDING = 10;

LegacyOptionsLayoutProfile layoutProfile(LegacyOptionsLayoutPreset preset)
{
    switch (preset)
    {
    case LegacyOptionsLayoutPreset::Compact:
        return {LEGACY_COMPACT_PANEL_WIDTH, legacyOptionContentPadding() * 2 + 96,
            legacyOptionContentPadding(), legacyOptionRowHeight(),
            legacyOptionRowSpacing(), legacyOptionContentPadding(), legacyOptionContentPadding(),
            LEGACY_OPTIONS_FOOTER_GAP, true};
    case LegacyOptionsLayoutPreset::Form:
        return {PLATFORM_LEGACY_CREATE_WORLD_PANEL_WIDTH, LEGACY_FORM_MIN_PANEL_WIDTH,
            LEGACY_FORM_CONTENT_PADDING, LEGACY_FORM_ROW_HEIGHT, LEGACY_FORM_ROW_SPACING,
            LEGACY_FORM_TOP_PADDING, LEGACY_FORM_BOTTOM_PADDING, LEGACY_OPTIONS_FOOTER_GAP, false};
    case LegacyOptionsLayoutPreset::Wide:
    default:
        return {legacyUiTheme().panelTargetWidth, 0, legacyOptionContentPadding(), legacyOptionRowHeight(),
            legacyOptionRowSpacing(), legacyOptionContentPadding(), legacyOptionContentPadding(),
            LEGACY_OPTIONS_FOOTER_GAP, true};
    }
}

// The theme's row metrics describe the desktop composition. A console screen has
// to fit the same panels under a proportionally taller logo, so the rows scale
// down with the screen. The floor keeps a low resolution readable: it is the
// tallest row that still lets the widest screen, Controls, page without running
// into the logo, and it clears the 12 px TickBox sprite.
int_t scaledRowHeight(int_t screenHeight, int_t reference)
{
    return legacyScaleToScreen(screenHeight, reference, 14, reference);
}

int_t scaledRowSpacing(int_t screenHeight, int_t reference)
{
    return legacyScaleToScreen(screenHeight, reference, 2, reference);
}

int_t panelWidthForProfile(const LegacyOptionsLayoutProfile &profile, LegacyOptionsLayoutPreset preset,
    int_t screenWidth)
{
    const int_t safeWidth = std::max<int_t>(1, screenWidth);
    if (preset == LegacyOptionsLayoutPreset::Form)
    {
        const int_t basePanelWidth = std::min<int_t>(profile.targetWidth,
            std::max<int_t>(LEGACY_FORM_BASE_MIN_PANEL_WIDTH, safeWidth - LEGACY_FORM_SCREEN_INSET));
        return std::max<int_t>(profile.minimumWidth, basePanelWidth - LEGACY_FORM_PANEL_REDUCTION);
    }

    int_t basePanelWidth = std::min<int_t>(legacyUiTheme().panelTargetWidth,
        std::max<int_t>(140, safeWidth - 16));
    basePanelWidth = std::min<int_t>(basePanelWidth, std::max<int_t>(1, safeWidth - 8));
    const int_t constrainedWidth = std::min<int_t>(basePanelWidth,
        std::min<int_t>(profile.targetWidth, safeWidth - 8));
    return profile.minimumWidth > 0
        ? std::max<int_t>(profile.minimumWidth, constrainedWidth)
        : constrainedWidth;
}
}

int_t legacyCenteredPanelY(int_t screenWidth, int_t screenHeight, int_t panelHeight, int_t footerGap)
{
    const int_t safeWidth = std::max<int_t>(1, screenWidth);
    const int_t safeHeight = std::max<int_t>(1, screenHeight);
    Minecraft *mc = Minecraft::getMinecraft();
    const bool is480 = (mc != nullptr && mc->displayHeight <= 480);
    const LegacySceneLayout scene = legacySceneLayout(safeWidth, safeHeight);
    const int_t availableTop = is480 ? 6 : scene.contentTop;
    const int_t footerTop = legacyHintRowY(safeHeight) - std::max<int_t>(0, footerGap);
    const int_t centeredPanelY = (safeHeight - panelHeight) / 2;
    const int_t maxPanelY = std::max<int_t>(availableTop, footerTop - panelHeight);
    return std::max<int_t>(availableTop, std::min<int_t>(centeredPanelY, maxPanelY));
}

LegacyOptionsLayout legacyOptionsLayout(int_t screenWidth, int_t screenHeight, int_t rowCount,
    LegacyOptionsLayoutPreset preset)
{
    LegacyOptionsLayout layout{};
    const int_t safeWidth = std::max<int_t>(1, screenWidth);
    const int_t safeHeight = std::max<int_t>(1, screenHeight);
    const int_t safeRows = std::max<int_t>(1, rowCount);
    const LegacyOptionsLayoutProfile profile = layoutProfile(preset);

    layout.panelWidth = panelWidthForProfile(profile, preset, safeWidth);
    layout.panelX = (safeWidth - layout.panelWidth) / 2;
    layout.contentX = layout.panelX + profile.contentPadding;
    layout.contentWidth = std::max<int_t>(1, layout.panelWidth - profile.contentPadding * 2);

    layout.rowHeight = profile.scaleRows ? scaledRowHeight(safeHeight, profile.rowHeight) : profile.rowHeight;
    layout.rowSpacing = profile.scaleRows ? scaledRowSpacing(safeHeight, profile.rowSpacing) : profile.rowSpacing;
    const int_t contentHeight = safeRows * layout.rowHeight + (safeRows - 1) * layout.rowSpacing;
    layout.panelHeight = profile.topPadding + contentHeight + profile.bottomPadding;

    const LegacySceneLayout scene = legacySceneLayout(safeWidth, safeHeight);
    layout.panelY = legacyCenteredPanelY(safeWidth, safeHeight, layout.panelHeight, profile.footerGap);
    layout.firstRowY = layout.panelY + profile.topPadding;

    layout.titleY = scene.titleY;
    layout.titleMaxWidth = scene.titleMaxWidth;
    layout.titleMaxHeight = scene.titleMaxHeight;
    return layout;
}

int_t legacyOptionsPanelWidth(int_t screenWidth, LegacyOptionsLayoutPreset preset)
{
    return panelWidthForProfile(layoutProfile(preset), preset, screenWidth);
}

int_t legacyOptionsMaxRows(int_t screenWidth, int_t screenHeight, LegacyOptionsLayoutPreset preset)
{
    const int_t safeHeight = std::max<int_t>(1, screenHeight);
    const LegacyOptionsLayoutProfile profile = layoutProfile(preset);
    const int_t rowHeight = profile.scaleRows ? scaledRowHeight(safeHeight, profile.rowHeight) : profile.rowHeight;
    const int_t rowSpacing = profile.scaleRows ? scaledRowSpacing(safeHeight, profile.rowSpacing) : profile.rowSpacing;

    Minecraft *mc = Minecraft::getMinecraft();
    const bool is480 = (mc != nullptr && mc->displayHeight <= 480);
    const LegacySceneLayout scene = legacySceneLayout(std::max<int_t>(1, screenWidth), safeHeight);
    const int_t contentTop = is480 ? 6 : scene.contentTop;
    // Same budget legacyOptionsLayout() centers within: below the title and above
    // the bottom hint row.
    const int_t availableBottom = legacyHintRowY(safeHeight) - profile.footerGap;
    const int_t available = availableBottom - contentTop - profile.topPadding - profile.bottomPadding;
    if (available < rowHeight)
        return 1;
    return std::max<int_t>(1, (available + rowSpacing) / (rowHeight + rowSpacing));
}
