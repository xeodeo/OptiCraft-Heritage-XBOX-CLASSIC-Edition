#pragma once

#include "Item.h"

class ItemCoal : public Item {
public:
    ItemCoal(int id);

    std::string getItemNameIS(ItemStack* itemstack) override;
    void getSubItems(int id, CreativeTabs *tab, std::vector<ItemStack *> &list) override;
};
