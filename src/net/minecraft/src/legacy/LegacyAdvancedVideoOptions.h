#pragma once

#include <vector>
#include "LegacyOptionsScreen.h"

class EnumOptions;
class GuiButton;

class LegacyAdvancedVideoOptions : public LegacyOptionsScreen
{
public:
    enum class Category
    {
        Hub,
        Details,
        Animations,
        Quality,
        Performance,
        Other
    };

    LegacyAdvancedVideoOptions(GuiScreen *parent, GameSettings *settings,
        LegacyOptionsBackgroundMode backgroundMode = LegacyOptionsBackgroundMode::Panorama,
        Category category = Category::Hub);

    void initGui() override;
    void drawScreen(int_t mouseX, int_t mouseY, float_t partialTick) override;

    void focusButtonById(int_t buttonId);

protected:
    void actionPerformed(GuiButton *button) override;

private:
    struct ItemEntry
    {
        int_t buttonId;
        EnumOptions *option;
    };

    void buildCategoryItems();
    void rebuildControls(int_t preferredButtonId = -1);
    int_t pageCount() const;
    void changePage(int_t delta, int_t preferredButtonId);

    Category category;
    std::vector<ItemEntry> items;
    int_t currentPage;
    int_t visibleRows;
    GuiButton *previousButton;
    GuiButton *nextButton;
    GuiButton *doneButton;
};
