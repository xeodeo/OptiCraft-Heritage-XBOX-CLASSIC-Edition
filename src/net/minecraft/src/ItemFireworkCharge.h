#pragma once

#include "Item.h"

class NBTBase;
class NBTTagCompound;

// net.minecraft.src.ItemFireworkCharge
class ItemFireworkCharge : public Item
{
public:
    explicit ItemFireworkCharge(int id);

    bool func_46058_c() override;
    int func_46057_a(int damage, int renderPass) override;
    int getColorFromItemStack(ItemStack *itemstack, int renderPass) override;
    void addInformation(ItemStack *itemstack, std::vector<std::string> &information) override;

    static NBTBase *getExplosionTag(ItemStack *itemstack, const std::string &key);
    static void addExplosionInfo(NBTTagCompound *explosion, std::vector<std::string> &information);
};
