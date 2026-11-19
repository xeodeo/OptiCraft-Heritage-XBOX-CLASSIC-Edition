#include "net/minecraft/src/UiStrings.h"
#include "LegacyVideoOptions.h"

#include <algorithm>
#include <vector>

#include "LegacyGuiButton.h"
#include "LegacyOptionCheckbox.h"
#include "LegacyOptionSlider.h"
#include "LegacyOptionState.h"
#include "LegacyOptionMetrics.h"
#include "net/minecraft/src/EnumOptions.h"
#include "net/minecraft/src/GameSettings.h"
#include "net/minecraft/src/GuiButton.h"
#include "net/minecraft/src/Minecraft.h"
#include "platform/PlatformConfig.h"
#include "platform/PlatformUserSettings.h"

namespace
{
enum LegacyVideoButtonId
{
    BUTTON_RENDER_DISTANCE = 300,
    BUTTON_GRAPHICS = 301,
    BUTTON_SMOOTH_LIGHTING = 302,
    BUTTON_VIEW_BOBBING = 303,
    BUTTON_CLOUDS = 304,
    BUTTON_FOG = 305,
    BUTTON_BRIGHTNESS = 306,
    BUTTON_DEFLICKER = 307,
    BUTTON_FRAME_CAP = 308,
    BUTTON_COORDINATES = 309,
    BUTTON_FULLBRIGHT = 310,
    BUTTON_OPAQUE_ICE = 311,
    BUTTON_POCKET_OPTIMIZATIONS = 312,
    BUTTON_PREV_PAGE = 397,
    BUTTON_NEXT_PAGE = 398,
    BUTTON_DONE = 399
};

}

LegacyVideoOptions::LegacyVideoOptions(GuiScreen *parent, GameSettings *settingsValue,
    LegacyOptionsBackgroundMode backgroundModeValue)
    : LegacyOptionsScreen(parent, settingsValue, backgroundModeValue), graphicsCheckbox(nullptr), smoothLightingCheckbox(nullptr),
      viewBobbingCheckbox(nullptr), cloudsCheckbox(nullptr), fogCheckbox(nullptr), deflickerCheckbox(nullptr),
      frameCapCheckbox(nullptr), coordinatesCheckbox(nullptr), fullbrightCheckbox(nullptr),
      opaqueIceCheckbox(nullptr), pocketOptimizationsCheckbox(nullptr), page(0)
{
}

void LegacyVideoOptions::initGui()
{
    rebuildControls();
}

void LegacyVideoOptions::rebuildControls()
{
    clearControlList();
    graphicsCheckbox = nullptr;
    smoothLightingCheckbox = nullptr;
    viewBobbingCheckbox = nullptr;
    cloudsCheckbox = nullptr;
    fogCheckbox = nullptr;
    deflickerCheckbox = nullptr;
    frameCapCheckbox = nullptr;
    coordinatesCheckbox = nullptr;
    fullbrightCheckbox = nullptr;
    opaqueIceCheckbox = nullptr;
    pocketOptimizationsCheckbox = nullptr;

    std::vector<int_t> items;
    items.push_back(BUTTON_GRAPHICS);
    items.push_back(BUTTON_SMOOTH_LIGHTING);
    items.push_back(BUTTON_VIEW_BOBBING);
#if !(PLATFORM_PS2 || PLATFORM_WII)
    items.push_back(BUTTON_CLOUDS);
    items.push_back(BUTTON_FOG);
#endif
#if PLATFORM_WII
    items.push_back(BUTTON_DEFLICKER);
#endif
#if PLATFORM_XBOX
    items.push_back(BUTTON_FRAME_CAP);
    items.push_back(BUTTON_COORDINATES);
    items.push_back(BUTTON_FULLBRIGHT);
#endif
#if !(PLATFORM_PS2 || PLATFORM_WII)
    items.push_back(BUTTON_OPAQUE_ICE);
    items.push_back(BUTTON_POCKET_OPTIMIZATIONS);
#endif
    items.push_back(BUTTON_RENDER_DISTANCE);
    items.push_back(BUTTON_BRIGHTNESS);

    const int_t totalItems = static_cast<int_t>(items.size());
    const int_t maxRows = legacyOptionsMaxRows(width, height, LegacyOptionsLayoutPreset::Compact);
    const bool paginated = totalItems + 1 > maxRows;
    const int_t rowsPerPage = paginated ? std::max<int_t>(1, maxRows - 2) : totalItems;
    const int_t pageCount = std::max<int_t>(1, (totalItems + rowsPerPage - 1) / rowsPerPage);
    if (page < 0)
        page = 0;
    if (page >= pageCount)
        page = pageCount - 1;

    const int_t startIndex = page * rowsPerPage;
    const int_t endIndex = std::min<int_t>(totalItems, startIndex + rowsPerPage);
    const int_t visibleItems = endIndex - startIndex;
    const int_t rowCount = paginated ? (visibleItems + 2) : (visibleItems + 1);

    configureLegacyLayout(rowCount, true, LegacyOptionsLayoutPreset::Compact);
    const int_t x = legacyLayout.contentX;
    const int_t w = legacyLayout.contentWidth;
    const int_t h = legacyLayout.rowHeight;
    int_t row = 0;

    for (int_t i = startIndex; i < endIndex; ++i)
    {
        const int_t y = legacyLayout.rowY(row++);
        switch (items[static_cast<std::size_t>(i)])
        {
        case BUTTON_GRAPHICS:
            graphicsCheckbox = new LegacyOptionCheckbox(BUTTON_GRAPHICS, x, y, w, h,
                uiText("Fancy Graphics"), settings->fancyGraphics);
            controlList.push_back(graphicsCheckbox);
            break;
        case BUTTON_SMOOTH_LIGHTING:
            smoothLightingCheckbox = new LegacyOptionCheckbox(BUTTON_SMOOTH_LIGHTING, x, y, w, h,
                uiText("Smooth Lighting"), legacySmoothLightingChecked(settings->ofAoLevel));
            controlList.push_back(smoothLightingCheckbox);
            break;
        case BUTTON_VIEW_BOBBING:
            viewBobbingCheckbox = new LegacyOptionCheckbox(BUTTON_VIEW_BOBBING, x, y, w, h,
                uiText("View Bobbing"), settings->viewBobbing);
            controlList.push_back(viewBobbingCheckbox);
            break;
        case BUTTON_CLOUDS:
            cloudsCheckbox = new LegacyOptionCheckbox(BUTTON_CLOUDS, x, y, w, h,
                uiText("Render Clouds"), legacyCloudsChecked(settings->ofClouds));
            controlList.push_back(cloudsCheckbox);
            break;
        case BUTTON_FOG:
            fogCheckbox = new LegacyOptionCheckbox(BUTTON_FOG, x, y, w, h,
                uiText("Fog"), legacyFogChecked(settings->ofFogOff));
            controlList.push_back(fogCheckbox);
            break;
        case BUTTON_DEFLICKER:
            deflickerCheckbox = new LegacyOptionCheckbox(BUTTON_DEFLICKER, x, y, w, h,
                uiText("Deflicker Filter"), settings->wiiDeflicker);
            controlList.push_back(deflickerCheckbox);
            break;
        case BUTTON_FRAME_CAP:
            frameCapCheckbox = new LegacyOptionCheckbox(BUTTON_FRAME_CAP, x, y, w, h,
                uiText("30 FPS Limit"), settings->limitFramerate == 2);
            controlList.push_back(frameCapCheckbox);
            break;
        case BUTTON_COORDINATES:
            coordinatesCheckbox = new LegacyOptionCheckbox(BUTTON_COORDINATES, x, y, w, h,
                uiText("Show Coordinates"), settings->showCoordinates);
            controlList.push_back(coordinatesCheckbox);
            break;
        case BUTTON_FULLBRIGHT:
            fullbrightCheckbox = new LegacyOptionCheckbox(BUTTON_FULLBRIGHT, x, y, w, h,
                uiText("Fullbright"), settings->fullbright);
            controlList.push_back(fullbrightCheckbox);
            break;
        case BUTTON_OPAQUE_ICE:
            opaqueIceCheckbox = new LegacyOptionCheckbox(BUTTON_OPAQUE_ICE, x, y, w, h,
                uiText("Opaque Ice"), settings->ofOpaqueIce);
            controlList.push_back(opaqueIceCheckbox);
            break;
        case BUTTON_POCKET_OPTIMIZATIONS:
            pocketOptimizationsCheckbox = new LegacyOptionCheckbox(BUTTON_POCKET_OPTIMIZATIONS, x, y, w, h,
                uiText("Pocket Optimizations"), settings->ofPocketOptimizations);
            controlList.push_back(pocketOptimizationsCheckbox);
            break;
        case BUTTON_RENDER_DISTANCE:
            controlList.push_back(new LegacyOptionSlider(BUTTON_RENDER_DISTANCE, x, y, w, h,
                settings, EnumOptions::RENDER_DISTANCE_FINE));
            break;
        case BUTTON_BRIGHTNESS:
            controlList.push_back(new LegacyOptionSlider(BUTTON_BRIGHTNESS, x, y, w, h,
                settings, EnumOptions::BRIGHTNESS));
            break;
        default:
            break;
        }
    }

    if (paginated)
    {
        const int_t navY = legacyLayout.rowY(row++);
        const int_t gap = 2;
        const int_t halfW = (w - gap) / 2;
        LegacyGuiButton *prevBtn = new LegacyGuiButton(BUTTON_PREV_PAGE, x, navY, halfW, h, uiText("Previous"));
        LegacyGuiButton *nextBtn = new LegacyGuiButton(BUTTON_NEXT_PAGE, x + halfW + gap, navY, w - halfW - gap, h, uiText("Next"));
        prevBtn->enabled = page > 0;
        nextBtn->enabled = page + 1 < pageCount;
        controlList.push_back(prevBtn);
        controlList.push_back(nextBtn);
    }

    controlList.push_back(new LegacyGuiButton(BUTTON_DONE, x, legacyLayout.rowY(row), w, h, uiText("Done")));
    syncLegacySelection();
}

void LegacyVideoOptions::syncCheckboxes()
{
    if (graphicsCheckbox != nullptr)
        graphicsCheckbox->setChecked(settings->fancyGraphics);
    if (smoothLightingCheckbox != nullptr)
        smoothLightingCheckbox->setChecked(legacySmoothLightingChecked(settings->ofAoLevel));
    if (viewBobbingCheckbox != nullptr)
        viewBobbingCheckbox->setChecked(settings->viewBobbing);
    if (cloudsCheckbox != nullptr)
        cloudsCheckbox->setChecked(legacyCloudsChecked(settings->ofClouds));
    if (fogCheckbox != nullptr)
        fogCheckbox->setChecked(legacyFogChecked(settings->ofFogOff));
    if (deflickerCheckbox != nullptr)
        deflickerCheckbox->setChecked(settings->wiiDeflicker);
    if (frameCapCheckbox != nullptr)
        frameCapCheckbox->setChecked(settings->limitFramerate == 2);
    if (coordinatesCheckbox != nullptr)
        coordinatesCheckbox->setChecked(settings->showCoordinates);
    if (fullbrightCheckbox != nullptr)
        fullbrightCheckbox->setChecked(settings->fullbright);
    if (opaqueIceCheckbox != nullptr)
        opaqueIceCheckbox->setChecked(settings->ofOpaqueIce);
    if (pocketOptimizationsCheckbox != nullptr)
        pocketOptimizationsCheckbox->setChecked(settings->ofPocketOptimizations);
}

void LegacyVideoOptions::actionPerformed(GuiButton *button)
{
    if (button == nullptr || !button->enabled)
        return;

    switch (button->id)
    {
    case BUTTON_GRAPHICS:
        settings->setOptionValue(EnumOptions::GRAPHICS, 1);
        syncCheckboxes();
        return;
    case BUTTON_SMOOTH_LIGHTING:
        settings->setOptionFloatValue(EnumOptions::AO_LEVEL,
            legacySmoothLightingToggleValue(settings->ofAoLevel));
        syncCheckboxes();
        return;
    case BUTTON_VIEW_BOBBING:
        settings->setOptionValue(EnumOptions::VIEW_BOBBING, 1);
        syncCheckboxes();
        return;
    case BUTTON_CLOUDS:
        settings->ofClouds = legacyCloudsToggledValue(settings->ofClouds);
        settings->saveOptions();
        syncCheckboxes();
        return;
    case BUTTON_FOG:
        settings->ofFogOff = legacyFogToggledOff(settings->ofFogOff);
        settings->saveOptions();
        syncCheckboxes();
        return;
    case BUTTON_DEFLICKER:
        settings->wiiDeflicker = !settings->wiiDeflicker;
        PlatformUserSettings::setDisplayDeflicker(settings->wiiDeflicker);
        settings->saveOptions();
        syncCheckboxes();
        return;
    case BUTTON_FRAME_CAP:
        settings->limitFramerate = settings->limitFramerate == 2 ? 0 : 2;
        settings->saveOptions();
        syncCheckboxes();
        return;
    case BUTTON_COORDINATES:
        settings->showCoordinates = !settings->showCoordinates;
        settings->saveOptions();
        syncCheckboxes();
        return;
    case BUTTON_FULLBRIGHT:
        settings->fullbright = !settings->fullbright;
        settings->saveOptions();
        syncCheckboxes();
        return;
    case BUTTON_OPAQUE_ICE:
        settings->setOptionValue(EnumOptions::OPAQUE_ICE, 1);
        syncCheckboxes();
        return;
    case BUTTON_POCKET_OPTIMIZATIONS:
        settings->setOptionValue(EnumOptions::POCKET_OPTIMIZATIONS, 1);
        syncCheckboxes();
        return;
    case BUTTON_PREV_PAGE:
        if (page > 0)
        {
            --page;
            selectedControlIndex = 0;
            rebuildControls();
        }
        return;
    case BUTTON_NEXT_PAGE:
        ++page;
        selectedControlIndex = 0;
        rebuildControls();
        return;
    case BUTTON_DONE:
        returnToParent();
        return;
    default:
        return;
    }
}

void LegacyVideoOptions::drawScreen(int_t mouseX, int_t mouseY, float_t partialTick)
{
    drawLegacyBackground(partialTick);
    updateLegacyPointerHover(mouseX, mouseY);
    GuiScreen::drawScreen(mouseX, mouseY, partialTick);
}
