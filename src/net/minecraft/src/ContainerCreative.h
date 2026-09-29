#pragma once

#include "Container.h"
#include <vector>

class EntityPlayer;
class ItemStack;

enum CreativeTabCategory
{
    CREATIVE_TAB_BUILDING = 0,
    CREATIVE_TAB_DECORATION,
    CREATIVE_TAB_REDSTONE_TRANSPORT,
    CREATIVE_TAB_MATERIALS,
    CREATIVE_TAB_FOOD_COMBAT,
    CREATIVE_TAB_ALL,
    CREATIVE_TAB_COUNT
};

// net.minecraft.src.ContainerCreative
class ContainerCreative : public Container
{
public:
    explicit ContainerCreative(EntityPlayer *player);
    ~ContainerCreative() override;

    bool isUsableByPlayer(EntityPlayer *player) override;
    void scrollTo(float_t scroll);

    void setCategory(int_t tabIndex);
    int_t getCategory() const { return currentTab; }

protected:
    void retrySlotClick(int_t slot, int_t button, bool shift, EntityPlayer *player) override;

public:
    std::vector<ItemStack *> itemList;
    std::vector<ItemStack *> masterList;
    std::vector<ItemStack *> tabItems[CREATIVE_TAB_COUNT];
    int_t currentTab;
};

