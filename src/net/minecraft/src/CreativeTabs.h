#pragma once

#include <string>
#include <vector>
#include "EnumEnchantmentType.h"

class Item;
class ItemStack;

// net.minecraft.src.CreativeTabs
class CreativeTabs
{
public:
    static CreativeTabs *creativeTabArray[12];
    static CreativeTabs *tabBlock;
    static CreativeTabs *tabDecorations;
    static CreativeTabs *tabRedstone;
    static CreativeTabs *tabTransport;
    static CreativeTabs *tabMisc;
    static CreativeTabs *tabAllSearch;
    static CreativeTabs *tabFood;
    static CreativeTabs *tabTools;
    static CreativeTabs *tabCombat;
    static CreativeTabs *tabBrewing;
    static CreativeTabs *tabMaterials;
    static CreativeTabs *tabInventory;

    static void initialize();
    static void cleanup();

    CreativeTabs(int tabIndex, const std::string &tabLabel);
    virtual ~CreativeTabs() = default;

    int getTabIndex() const { return tabIndex; }
    const std::string &getTabLabel() const { return tabLabel; }
    std::string getTranslatedTabLabel() const;

    Item *getTabIconItem();
    virtual int getTabIconItemIndex();

    const std::string &getBackgroundImageName() const { return backgroundImageName; }
    CreativeTabs *setBackgroundImageName(const std::string &name) { backgroundImageName = name; return this; }

    bool drawInForegroundOfTab() const { return drawTitle; }
    CreativeTabs *setNoTitle() { drawTitle = false; return this; }

    bool shouldHidePlayerInventory() const { return hasScrollbar; }
    CreativeTabs *setNoScrollbar() { hasScrollbar = false; return this; }

    int getTabColumn() const { return tabIndex % 6; }
    bool isTabInFirstRow() const { return tabIndex < 6; }

    virtual void displayAllReleventItems(std::vector<ItemStack *> &list);
    void func_92116_a(std::vector<ItemStack *> &list, const std::vector<EnumEnchantmentType> &types);

private:
    int tabIndex;
    std::string tabLabel;
    std::string backgroundImageName;
    bool hasScrollbar;
    bool drawTitle;
};

class CreativeTabBlock : public CreativeTabs
{
public:
    CreativeTabBlock(int index, const std::string &label);
    int getTabIconItemIndex() override;
};

class CreativeTabDeco : public CreativeTabs
{
public:
    CreativeTabDeco(int index, const std::string &label);
    int getTabIconItemIndex() override;
};

class CreativeTabRedstone : public CreativeTabs
{
public:
    CreativeTabRedstone(int index, const std::string &label);
    int getTabIconItemIndex() override;
};

class CreativeTabTransport : public CreativeTabs
{
public:
    CreativeTabTransport(int index, const std::string &label);
    int getTabIconItemIndex() override;
};

class CreativeTabMisc : public CreativeTabs
{
public:
    CreativeTabMisc(int index, const std::string &label);
    int getTabIconItemIndex() override;
    void displayAllReleventItems(std::vector<ItemStack *> &list) override;
};

class CreativeTabSearch : public CreativeTabs
{
public:
    CreativeTabSearch(int index, const std::string &label);
    int getTabIconItemIndex() override;
};

class CreativeTabFood : public CreativeTabs
{
public:
    CreativeTabFood(int index, const std::string &label);
    int getTabIconItemIndex() override;
};

class CreativeTabTools : public CreativeTabs
{
public:
    CreativeTabTools(int index, const std::string &label);
    int getTabIconItemIndex() override;
    void displayAllReleventItems(std::vector<ItemStack *> &list) override;
};

class CreativeTabCombat : public CreativeTabs
{
public:
    CreativeTabCombat(int index, const std::string &label);
    int getTabIconItemIndex() override;
    void displayAllReleventItems(std::vector<ItemStack *> &list) override;
};

class CreativeTabBrewing : public CreativeTabs
{
public:
    CreativeTabBrewing(int index, const std::string &label);
    int getTabIconItemIndex() override;
};

class CreativeTabMaterial : public CreativeTabs
{
public:
    CreativeTabMaterial(int index, const std::string &label);
    int getTabIconItemIndex() override;
};

class CreativeTabInventory : public CreativeTabs
{
public:
    CreativeTabInventory(int index, const std::string &label);
    int getTabIconItemIndex() override;
};
