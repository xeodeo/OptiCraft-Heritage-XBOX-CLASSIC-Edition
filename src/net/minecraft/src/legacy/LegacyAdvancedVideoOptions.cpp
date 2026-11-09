#include "LegacyAdvancedVideoOptions.h"

#include <algorithm>
#include <cmath>
#include <string>

#include "LegacyGuiButton.h"
#include "LegacyOptionCheckbox.h"
#include "LegacyOptionSlider.h"
#include "LegacyOptionsLayout.h"
#include "net/minecraft/src/Config.h"
#include "net/minecraft/src/EnumOptions.h"
#include "net/minecraft/src/GameSettings.h"
#include "net/minecraft/src/GuiButton.h"
#include "net/minecraft/src/Minecraft.h"
#include "net/minecraft/src/SoundManager.h"
#include "net/minecraft/src/StringTranslate.h"
#include "net/minecraft/src/UiStrings.h"
#include "platform/PlatformConfig.h"
#include "platform/RenderAPI.h"

namespace
{
enum LegacyAdvancedVideoButtonId
{
    BUTTON_CAT_DETAILS = 500,
    BUTTON_CAT_ANIMATIONS = 501,
    BUTTON_CAT_QUALITY = 502,
    BUTTON_CAT_PERFORMANCE = 503,
    BUTTON_CAT_OTHER = 504,

    BUTTON_ALL_ON = 510,
    BUTTON_ALL_OFF = 511,

    BUTTON_PREV_PAGE = 520,
    BUTTON_NEXT_PAGE = 521,
    BUTTON_DONE = 599,

    BUTTON_OPTION_BASE = 600
};

bool isBooleanCheckboxOption(const EnumOptions *option)
{
    return option == EnumOptions::SKY ||
           option == EnumOptions::STARS ||
           option == EnumOptions::SUN_MOON ||
           option == EnumOptions::DEPTH_FOG ||
           option == EnumOptions::ANIMATED_FIRE ||
           option == EnumOptions::ANIMATED_PORTAL ||
           option == EnumOptions::ANIMATED_REDSTONE ||
           option == EnumOptions::ANIMATED_EXPLOSION ||
           option == EnumOptions::ANIMATED_FLAME ||
           option == EnumOptions::ANIMATED_SMOKE ||
           option == EnumOptions::VOID_PARTICLES ||
           option == EnumOptions::WATER_PARTICLES ||
           option == EnumOptions::RAIN_SPLASH ||
           option == EnumOptions::PORTAL_PARTICLES ||
           option == EnumOptions::DRIPPING_WATER_LAVA ||
           option == EnumOptions::ANIMATED_TERRAIN ||
           option == EnumOptions::ANIMATED_ITEMS ||
           option == EnumOptions::ANIMATED_TEXTURES ||
           option == EnumOptions::CLEAR_WATER ||
           option == EnumOptions::RANDOM_MOBS ||
           option == EnumOptions::BETTER_SNOW ||
           option == EnumOptions::CUSTOM_FONTS ||
           option == EnumOptions::CUSTOM_COLORS ||
           option == EnumOptions::SWAMP_COLORS ||
           option == EnumOptions::SMOOTH_BIOMES ||
           option == EnumOptions::NATURAL_TEXTURES ||
           option == EnumOptions::SMOOTH_FPS ||
           option == EnumOptions::SMOOTH_INPUT ||
           option == EnumOptions::LOAD_FAR ||
           option == EnumOptions::CHUNK_UPDATES_DYNAMIC ||
           option == EnumOptions::FAR_VIEW ||
           option == EnumOptions::FAST_DEBUG_INFO ||
           option == EnumOptions::PROFILER ||
           option == EnumOptions::WEATHER ||
           option == EnumOptions::ANAGLYPH;
}

bool getBooleanOptionValue(const GameSettings *settings, const EnumOptions *option)
{
    if (settings == nullptr || option == nullptr)
        return false;
    if (option == EnumOptions::SKY) return settings->ofSky;
    if (option == EnumOptions::STARS) return settings->ofStars;
    if (option == EnumOptions::SUN_MOON) return settings->ofSunMoon;
    if (option == EnumOptions::DEPTH_FOG) return settings->ofDepthFog;
    if (option == EnumOptions::ANIMATED_FIRE) return settings->ofAnimatedFire;
    if (option == EnumOptions::ANIMATED_PORTAL) return settings->ofAnimatedPortal;
    if (option == EnumOptions::ANIMATED_REDSTONE) return settings->ofAnimatedRedstone;
    if (option == EnumOptions::ANIMATED_EXPLOSION) return settings->ofAnimatedExplosion;
    if (option == EnumOptions::ANIMATED_FLAME) return settings->ofAnimatedFlame;
    if (option == EnumOptions::ANIMATED_SMOKE) return settings->ofAnimatedSmoke;
    if (option == EnumOptions::VOID_PARTICLES) return settings->ofVoidParticles;
    if (option == EnumOptions::WATER_PARTICLES) return settings->ofWaterParticles;
    if (option == EnumOptions::RAIN_SPLASH) return settings->ofRainSplash;
    if (option == EnumOptions::PORTAL_PARTICLES) return settings->ofPortalParticles;
    if (option == EnumOptions::DRIPPING_WATER_LAVA) return settings->ofDrippingWaterLava;
    if (option == EnumOptions::ANIMATED_TERRAIN) return settings->ofAnimatedTerrain;
    if (option == EnumOptions::ANIMATED_ITEMS) return settings->ofAnimatedItems;
    if (option == EnumOptions::ANIMATED_TEXTURES) return settings->ofAnimatedTextures;
    if (option == EnumOptions::CLEAR_WATER) return settings->ofClearWater;
    if (option == EnumOptions::RANDOM_MOBS) return settings->ofRandomMobs;
    if (option == EnumOptions::BETTER_SNOW) return settings->ofBetterSnow;
    if (option == EnumOptions::CUSTOM_FONTS) return settings->ofCustomFonts;
    if (option == EnumOptions::CUSTOM_COLORS) return settings->ofCustomColors;
    if (option == EnumOptions::SWAMP_COLORS) return settings->ofSwampColors;
    if (option == EnumOptions::SMOOTH_BIOMES) return settings->ofSmoothBiomes;
    if (option == EnumOptions::NATURAL_TEXTURES) return settings->ofNaturalTextures;
    if (option == EnumOptions::SMOOTH_FPS) return settings->ofSmoothFps;
    if (option == EnumOptions::SMOOTH_INPUT) return settings->ofSmoothInput;
    if (option == EnumOptions::LOAD_FAR) return settings->ofLoadFar;
    if (option == EnumOptions::CHUNK_UPDATES_DYNAMIC) return settings->ofChunkUpdatesDynamic;
    if (option == EnumOptions::FAR_VIEW) return settings->ofFarView;
    if (option == EnumOptions::FAST_DEBUG_INFO) return settings->ofFastDebugInfo;
    if (option == EnumOptions::PROFILER) return settings->ofProfiler;
    if (option == EnumOptions::WEATHER) return settings->ofWeather;
    if (option == EnumOptions::ANAGLYPH) return settings->anaglyph;
    return false;
}

std::string checkboxLabelForOption(GameSettings *settings, const EnumOptions *option)
{
    if (settings == nullptr || option == nullptr)
        return "";
    return uiText(StringTranslate::getInstance()->translateKey(option->getEnumString()));
}

// Steps a multi-value option forward (direction > 0) or backward (direction < 0)
// while routing the final transition through GameSettings::setOptionValue so all
// chunk/texture invalidations and options.txt saves happen once through the
// canonical code path.
void cycleOptionValue(GameSettings *settings, const EnumOptions *option, int_t direction)
{
    if (settings == nullptr || option == nullptr || direction == 0)
        return;

    if (option == EnumOptions::PARTICLES)
    {
        settings->setOptionValue(option, direction > 0 ? 1 : -1);
        return;
    }

    if (direction < 0)
    {
        if (option == EnumOptions::CLOUDS)
            settings->ofClouds = (settings->ofClouds + 2) % 4;
        else if (option == EnumOptions::TREES)
            settings->ofTrees = (settings->ofTrees + 1) % 3;
        else if (option == EnumOptions::GRASS)
            settings->ofGrass = (settings->ofGrass + 1) % 3;
        else if (option == EnumOptions::WATER)
            settings->ofWater = (settings->ofWater + 1) % 3;
        else if (option == EnumOptions::RAIN)
            settings->ofRain = (settings->ofRain + 2) % 4;
        else if (option == EnumOptions::FOG_FANCY)
        {
            if (Config::isFancyFogAvailable())
            {
                // 3-state cycle: Fast -> Fancy -> OFF -> Fast.
                // Advance once without side effects so setOptionValue lands on the previous state.
                if (settings->ofFogOff)
                {
                    settings->ofFogOff = false;
                    settings->ofFogFancy = false;
                }
                else if (settings->ofFogFancy)
                {
                    settings->ofFogOff = true;
                    settings->ofFogFancy = false;
                }
                else
                {
                    settings->ofFogFancy = true;
                }
            }
        }
        else if (option == EnumOptions::FOG_START)
        {
            // 4 stops: 0.2, 0.4, 0.6, 0.8. Advance 2 stops so +0.2f lands on -0.2f.
            for (int_t step = 0; step < 2; ++step)
            {
                settings->ofFogStart += 0.2f;
                if (settings->ofFogStart > 0.81f)
                    settings->ofFogStart = 0.2f;
            }
        }
        else if (option == EnumOptions::ANIMATED_WATER)
            settings->ofAnimatedWater = (settings->ofAnimatedWater + 1) % 3;
        else if (option == EnumOptions::ANIMATED_LAVA)
            settings->ofAnimatedLava = (settings->ofAnimatedLava + 1) % 3;
        else if (option == EnumOptions::BETTER_GRASS)
            settings->ofBetterGrass = (settings->ofBetterGrass % 3) + 1;
        else if (option == EnumOptions::CONNECTED_TEXTURES)
            settings->ofConnectedTextures = (settings->ofConnectedTextures % 3) + 1;
        else if (option == EnumOptions::MIPMAP_LEVEL)
            settings->ofMipmapLevel = (settings->ofMipmapLevel + 3) % 5;
        else if (option == EnumOptions::AA_LEVEL)
        {
            static const int_t levels[] = {0, 2, 4, 8, 16};
            int_t index = 0;
            while (index < 4 && levels[index] != settings->ofAaLevel) ++index;
            settings->ofAaLevel = levels[(index + 3) % 5];
        }
        else if (option == EnumOptions::AF_LEVEL)
        {
            for (int_t step = 0; step < 3; ++step)
            {
                settings->ofAfLevel *= 2;
                if (settings->ofAfLevel > 16)
                    settings->ofAfLevel = 1;
            }
        }
        else if (option == EnumOptions::PRELOADED_CHUNKS)
        {
            for (int_t step = 0; step < 3; ++step)
            {
                settings->ofPreloadedChunks += 2;
                if (settings->ofPreloadedChunks > 8)
                    settings->ofPreloadedChunks = 0;
            }
        }
        else if (option == EnumOptions::CHUNK_UPDATES)
        {
            for (int_t step = 0; step < 3; ++step)
            {
                settings->ofChunkUpdates++;
                if (settings->ofChunkUpdates > 5)
                    settings->ofChunkUpdates = 1;
            }
        }
        else if (option == EnumOptions::TIME)
            settings->ofTime = (settings->ofTime + 1) % 3;
        else if (option == EnumOptions::AUTOSAVE_TICKS)
        {
            for (int_t step = 0; step < 2; ++step)
            {
                settings->ofAutoSaveTicks *= 10;
                if (settings->ofAutoSaveTicks > 40000)
                    settings->ofAutoSaveTicks = 40;
            }
        }
    }

    settings->setOptionValue(option, 1);
}

class LegacyEnumCheckbox : public LegacyOptionCheckbox
{
public:
    LegacyEnumCheckbox(int_t id, int_t x, int_t y, int_t width, int_t height,
        GameSettings *settingsValue, EnumOptions *optionValue)
        : LegacyOptionCheckbox(id, x, y, width, height,
            checkboxLabelForOption(settingsValue, optionValue),
            getBooleanOptionValue(settingsValue, optionValue)),
          settings(settingsValue), option(optionValue)
    {
    }

    void refreshFromSettings()
    {
        if (settings != nullptr && option != nullptr)
            setChecked(getBooleanOptionValue(settings, option));
    }

    bool adjustKeyboard(Minecraft *mc, int_t direction) override
    {
        (void)mc;
        if (direction == 0 || settings == nullptr || option == nullptr)
            return false;
        settings->setOptionValue(option, 1);
        refreshFromSettings();
        return true;
    }

private:
    GameSettings *settings;
    EnumOptions *option;
};

class LegacyEnumCycleButton : public LegacyGuiButton
{
public:
    LegacyEnumCycleButton(int_t id, int_t x, int_t y, int_t width, int_t height,
        GameSettings *settingsValue, EnumOptions *optionValue)
        : LegacyGuiButton(id, x, y, width, height,
            settingsValue != nullptr && optionValue != nullptr ? settingsValue->getKeyBinding(optionValue) : ""),
          settings(settingsValue), option(optionValue)
    {
    }

    void refreshFromSettings()
    {
        if (settings != nullptr && option != nullptr)
            displayString = settings->getKeyBinding(option);
    }

    bool adjustKeyboard(Minecraft *mc, int_t direction) override
    {
        (void)mc;
        if (direction == 0 || settings == nullptr || option == nullptr)
            return false;
        cycleOptionValue(settings, option, direction);
        refreshFromSettings();
        return true;
    }

private:
    GameSettings *settings;
    EnumOptions *option;
};

class LegacyPagerButton : public LegacyGuiButton
{
public:
    LegacyPagerButton(int_t id, int_t x, int_t y, int_t width, int_t height, const std::string &text,
        LegacyAdvancedVideoOptions *ownerValue, int_t siblingIdValue, int_t siblingDirValue)
        : LegacyGuiButton(id, x, y, width, height, text),
          owner(ownerValue), siblingId(siblingIdValue), siblingDir(siblingDirValue)
    {
    }

    bool adjustKeyboard(Minecraft *mc, int_t direction) override
    {
        (void)mc;
        if (owner == nullptr || direction == 0)
            return false;
        if ((siblingDir < 0 && direction < 0) || (siblingDir > 0 && direction > 0))
        {
            owner->focusButtonById(siblingId);
            return true;
        }
        return false;
    }

private:
    LegacyAdvancedVideoOptions *owner;
    int_t siblingId;
    int_t siblingDir;
};
} // namespace

LegacyAdvancedVideoOptions::LegacyAdvancedVideoOptions(GuiScreen *parent, GameSettings *settingsValue,
    LegacyOptionsBackgroundMode backgroundModeValue, Category categoryValue)
    : LegacyOptionsScreen(parent, settingsValue, backgroundModeValue),
      category(categoryValue), currentPage(0), visibleRows(1),
      previousButton(nullptr), nextButton(nullptr), doneButton(nullptr)
{
}

void LegacyAdvancedVideoOptions::buildCategoryItems()
{
    items.clear();
    int_t nextId = BUTTON_OPTION_BASE;
    auto addOption = [&](EnumOptions *option)
    {
        if (option != nullptr)
            items.push_back({nextId++, option});
    };

    switch (category)
    {
    case Category::Details:
        addOption(EnumOptions::CLOUDS);
        addOption(EnumOptions::CLOUD_HEIGHT);
        addOption(EnumOptions::TREES);
        addOption(EnumOptions::GRASS);
        addOption(EnumOptions::WATER);
        addOption(EnumOptions::RAIN);
        addOption(EnumOptions::SKY);
        addOption(EnumOptions::STARS);
        addOption(EnumOptions::SUN_MOON);
        addOption(EnumOptions::DEPTH_FOG);
        addOption(EnumOptions::FOG_FANCY);
        addOption(EnumOptions::FOG_START);
        break;

    case Category::Animations:
        addOption(EnumOptions::ANIMATED_WATER);
        addOption(EnumOptions::ANIMATED_LAVA);
        addOption(EnumOptions::ANIMATED_FIRE);
        addOption(EnumOptions::ANIMATED_PORTAL);
        addOption(EnumOptions::ANIMATED_REDSTONE);
        addOption(EnumOptions::ANIMATED_EXPLOSION);
        addOption(EnumOptions::ANIMATED_FLAME);
        addOption(EnumOptions::ANIMATED_SMOKE);
        addOption(EnumOptions::VOID_PARTICLES);
        addOption(EnumOptions::WATER_PARTICLES);
        addOption(EnumOptions::RAIN_SPLASH);
        addOption(EnumOptions::PORTAL_PARTICLES);
        addOption(EnumOptions::DRIPPING_WATER_LAVA);
        addOption(EnumOptions::ANIMATED_TERRAIN);
        addOption(EnumOptions::ANIMATED_ITEMS);
        addOption(EnumOptions::ANIMATED_TEXTURES);
        addOption(EnumOptions::PARTICLES);
        items.push_back({BUTTON_ALL_ON, nullptr});
        items.push_back({BUTTON_ALL_OFF, nullptr});
        break;

    case Category::Quality:
        addOption(EnumOptions::AO_LEVEL);
        if (renderSupportsFeature(RenderFeature::Mipmaps))
        {
            addOption(EnumOptions::MIPMAP_LEVEL);
            addOption(EnumOptions::MIPMAP_TYPE);
        }
        if (renderSupportsFeature(RenderFeature::AnisotropicFiltering))
            addOption(EnumOptions::AF_LEVEL);
        if (renderSupportsFeature(RenderFeature::MultisampleAntialiasing))
            addOption(EnumOptions::AA_LEVEL);
        addOption(EnumOptions::CLEAR_WATER);
#if PLATFORM_OPTIFINE_RANDOM_MOBS
        addOption(EnumOptions::RANDOM_MOBS);
#endif
        addOption(EnumOptions::BETTER_GRASS);
        addOption(EnumOptions::BETTER_SNOW);
#if PLATFORM_OPTIFINE_CUSTOM_FONTS
        addOption(EnumOptions::CUSTOM_FONTS);
#endif
        addOption(EnumOptions::CUSTOM_COLORS);
        addOption(EnumOptions::SWAMP_COLORS);
        addOption(EnumOptions::SMOOTH_BIOMES);
        addOption(EnumOptions::CONNECTED_TEXTURES);
        addOption(EnumOptions::NATURAL_TEXTURES);
        break;

    case Category::Performance:
        addOption(EnumOptions::SMOOTH_FPS);
        addOption(EnumOptions::SMOOTH_INPUT);
        addOption(EnumOptions::LOAD_FAR);
        addOption(EnumOptions::PRELOADED_CHUNKS);
        addOption(EnumOptions::CHUNK_UPDATES);
        addOption(EnumOptions::CHUNK_UPDATES_DYNAMIC);
        addOption(EnumOptions::FAR_VIEW);
        if (Config::isOcclusionAvailable())
            addOption(EnumOptions::ADVANCED_OPENGL);
        break;

    case Category::Other:
        addOption(EnumOptions::FAST_DEBUG_INFO);
        addOption(EnumOptions::PROFILER);
        addOption(EnumOptions::WEATHER);
        addOption(EnumOptions::TIME);
        addOption(EnumOptions::AUTOSAVE_TICKS);
        addOption(EnumOptions::ANAGLYPH);
        break;

    case Category::Hub:
    default:
        break;
    }
}

int_t LegacyAdvancedVideoOptions::pageCount() const
{
    if (visibleRows <= 0 || items.empty())
        return 1;
    return (static_cast<int_t>(items.size()) + visibleRows - 1) / visibleRows;
}

void LegacyAdvancedVideoOptions::initGui()
{
    if (category == Category::Hub)
    {
        clearControlList();
        const int_t rowCount = 6;
        configureLegacyLayout(rowCount, true, LegacyOptionsLayoutPreset::Compact);
        const int_t x = legacyLayout.contentX;
        const int_t w = legacyLayout.contentWidth;
        const int_t h = legacyLayout.rowHeight;

        controlList.push_back(new LegacyGuiButton(BUTTON_CAT_DETAILS, x, legacyLayout.rowY(0), w, h, uiText("Details...")));
        controlList.push_back(new LegacyGuiButton(BUTTON_CAT_ANIMATIONS, x, legacyLayout.rowY(1), w, h, uiText("Animations...")));
        controlList.push_back(new LegacyGuiButton(BUTTON_CAT_QUALITY, x, legacyLayout.rowY(2), w, h, uiText("Quality...")));
        controlList.push_back(new LegacyGuiButton(BUTTON_CAT_PERFORMANCE, x, legacyLayout.rowY(3), w, h, uiText("Performance...")));
        controlList.push_back(new LegacyGuiButton(BUTTON_CAT_OTHER, x, legacyLayout.rowY(4), w, h, uiText("Other...")));
        controlList.push_back(new LegacyGuiButton(BUTTON_DONE, x, legacyLayout.rowY(5), w, h, uiText("Done")));
        return;
    }

    buildCategoryItems();
    rebuildControls();
}

void LegacyAdvancedVideoOptions::rebuildControls(int_t preferredButtonId)
{
    clearControlList();
    previousButton = nullptr;
    nextButton = nullptr;
    doneButton = nullptr;

    const int_t maxTotalRows = std::max<int_t>(4, std::min<int_t>(10,
        legacyOptionsMaxRows(width, height, LegacyOptionsLayoutPreset::Wide)));
    const int_t itemCount = static_cast<int_t>(items.size());
    const bool needsPaging = itemCount > (maxTotalRows - 1);

    if (needsPaging)
    {
        visibleRows = std::max<int_t>(1, maxTotalRows - 2);
        const int_t pages = pageCount();
        currentPage = std::max<int_t>(0, std::min<int_t>(currentPage, pages - 1));
    }
    else
    {
        visibleRows = std::max<int_t>(1, itemCount);
        currentPage = 0;
    }

    const int_t startIndex = currentPage * visibleRows;
    const int_t endIndex = std::min<int_t>(itemCount, startIndex + visibleRows);
    const int_t pageItemRows = std::max<int_t>(1, endIndex - startIndex);
    const int_t layoutRowCount = needsPaging ? (visibleRows + 2) : (pageItemRows + 1);

    configureLegacyLayout(layoutRowCount, true, LegacyOptionsLayoutPreset::Wide);
    const int_t x = legacyLayout.contentX;
    const int_t w = legacyLayout.contentWidth;
    const int_t h = legacyLayout.rowHeight;

    for (int_t i = startIndex; i < endIndex; ++i)
    {
        const int_t row = i - startIndex;
        const ItemEntry &entry = items[static_cast<std::size_t>(i)];
        const int_t rowY = legacyLayout.rowY(row);

        if (entry.buttonId == BUTTON_ALL_ON)
        {
            controlList.push_back(new LegacyGuiButton(BUTTON_ALL_ON, x, rowY, w, h, uiText("All ON")));
        }
        else if (entry.buttonId == BUTTON_ALL_OFF)
        {
            controlList.push_back(new LegacyGuiButton(BUTTON_ALL_OFF, x, rowY, w, h, uiText("All OFF")));
        }
        else if (entry.option != nullptr)
        {
            if (entry.option->getEnumFloat())
            {
                controlList.push_back(new LegacyOptionSlider(entry.buttonId, x, rowY, w, h, settings, entry.option));
            }
            else if (isBooleanCheckboxOption(entry.option))
            {
                controlList.push_back(new LegacyEnumCheckbox(entry.buttonId, x, rowY, w, h, settings, entry.option));
            }
            else
            {
                controlList.push_back(new LegacyEnumCycleButton(entry.buttonId, x, rowY, w, h, settings, entry.option));
            }
        }
    }

    if (needsPaging)
    {
        const int_t navRowY = legacyLayout.rowY(visibleRows);
        const int_t gap = 4;
        const int_t halfWidth = (w - gap) / 2;
        previousButton = new LegacyPagerButton(BUTTON_PREV_PAGE, x, navRowY, halfWidth, h,
            uiText("Previous"), this, BUTTON_NEXT_PAGE, 1);
        nextButton = new LegacyPagerButton(BUTTON_NEXT_PAGE, x + halfWidth + gap, navRowY, w - halfWidth - gap, h,
            uiText("Next"), this, BUTTON_PREV_PAGE, -1);
        previousButton->enabled = currentPage > 0;
        nextButton->enabled = currentPage + 1 < pageCount();
        controlList.push_back(previousButton);
        controlList.push_back(nextButton);
    }

    const int_t doneRow = needsPaging ? (visibleRows + 1) : pageItemRows;
    doneButton = new LegacyGuiButton(BUTTON_DONE, x, legacyLayout.rowY(doneRow), w, h, uiText("Done"));
    controlList.push_back(doneButton);

    if (preferredButtonId >= 0)
        focusButtonById(preferredButtonId);
}

void LegacyAdvancedVideoOptions::focusButtonById(int_t buttonId)
{
    for (int_t i = 0; i < static_cast<int_t>(controlList.size()); ++i)
    {
        GuiButton *btn = controlList[static_cast<std::size_t>(i)];
        if (btn != nullptr && btn->id == buttonId && btn->enabled && btn->enabled2)
        {
            selectedControlIndex = i;
            hoveredControlIndex = -1;
            syncLegacySelection();
            return;
        }
    }
    if (buttonId == BUTTON_PREV_PAGE && nextButton != nullptr && nextButton->enabled)
        focusButtonById(BUTTON_NEXT_PAGE);
    else if (buttonId == BUTTON_NEXT_PAGE && previousButton != nullptr && previousButton->enabled)
        focusButtonById(BUTTON_PREV_PAGE);
}

void LegacyAdvancedVideoOptions::changePage(int_t delta, int_t preferredButtonId)
{
    const int_t nextPage = std::max<int_t>(0, std::min<int_t>(pageCount() - 1, currentPage + delta));
    if (nextPage == currentPage)
        return;

    currentPage = nextPage;
    rebuildControls(preferredButtonId);
}

void LegacyAdvancedVideoOptions::actionPerformed(GuiButton *button)
{
    if (button == nullptr || !button->enabled)
        return;

    if (category == Category::Hub)
    {
        switch (button->id)
        {
        case BUTTON_CAT_DETAILS:
            mc->displayGuiScreen(new LegacyAdvancedVideoOptions(this, settings, backgroundMode, Category::Details));
            return;
        case BUTTON_CAT_ANIMATIONS:
            mc->displayGuiScreen(new LegacyAdvancedVideoOptions(this, settings, backgroundMode, Category::Animations));
            return;
        case BUTTON_CAT_QUALITY:
            mc->displayGuiScreen(new LegacyAdvancedVideoOptions(this, settings, backgroundMode, Category::Quality));
            return;
        case BUTTON_CAT_PERFORMANCE:
            mc->displayGuiScreen(new LegacyAdvancedVideoOptions(this, settings, backgroundMode, Category::Performance));
            return;
        case BUTTON_CAT_OTHER:
            mc->displayGuiScreen(new LegacyAdvancedVideoOptions(this, settings, backgroundMode, Category::Other));
            return;
        case BUTTON_DONE:
            returnToParent();
            return;
        default:
            return;
        }
    }

    if (button->id == BUTTON_PREV_PAGE)
    {
        changePage(-1, BUTTON_PREV_PAGE);
        return;
    }
    if (button->id == BUTTON_NEXT_PAGE)
    {
        changePage(1, BUTTON_NEXT_PAGE);
        return;
    }
    if (button->id == BUTTON_DONE)
    {
        returnToParent();
        return;
    }
    if (button->id == BUTTON_ALL_ON)
    {
        if (settings != nullptr)
            settings->setAllAnimations(true);
        rebuildControls(BUTTON_ALL_ON);
        return;
    }
    if (button->id == BUTTON_ALL_OFF)
    {
        if (settings != nullptr)
            settings->setAllAnimations(false);
        rebuildControls(BUTTON_ALL_OFF);
        return;
    }

    for (const ItemEntry &entry : items)
    {
        if (entry.buttonId == button->id && entry.option != nullptr)
        {
            if (entry.option->getEnumFloat())
                return;
            settings->setOptionValue(entry.option, 1);
            if (isBooleanCheckboxOption(entry.option))
            {
                static_cast<LegacyEnumCheckbox *>(button)->refreshFromSettings();
            }
            else
            {
                static_cast<LegacyEnumCycleButton *>(button)->refreshFromSettings();
            }
            return;
        }
    }
}

void LegacyAdvancedVideoOptions::drawScreen(int_t mouseX, int_t mouseY, float_t partialTick)
{
    drawLegacyBackground(partialTick);
    updateLegacyPointerHover(mouseX, mouseY);
    GuiScreen::drawScreen(mouseX, mouseY, partialTick);
}
