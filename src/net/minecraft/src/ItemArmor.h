#pragma once

#include "Item.h"
#include "EnumArmorMaterial.h"
#include <string>
#include <vector>

class ItemArmor : public Item {
public:
    static constexpr int DEFAULT_LEATHER_COLOR = 0xA06540;

    ItemArmor(int id, EnumArmorMaterial material, int renderIndex, int armorType);

    static const int maxDamageArray[4];

    int getItemEnchantability() override;
    // Mod armour (ChocoCraft's Chocodisguise): own texture for armour layer 1 or 2
    // instead of /armor/<material>_<layer>.png. nullptr = the vanilla texture.
    virtual const char *getCustomArmorTexture(int layer) const { (void)layer; return nullptr; }
    bool getIsRepairable(ItemStack* stack1, ItemStack* stack2) override;
    bool func_46058_c() override;
    int func_46057_a(int damage, int renderPass) override;
    int getColorFromItemStack(ItemStack* itemstack, int renderPass) override;
    void addInformation(ItemStack* itemstack, std::vector<std::string>& information) override;
    ItemStack* onItemRightClick(ItemStack* itemstack, World* world, EntityPlayer* entityplayer) override;

    bool hasColor(ItemStack* stack) const;
    int getColor(ItemStack* stack) const;
    void setColor(ItemStack* stack, int color);
    void removeColor(ItemStack* stack);

    EnumArmorMaterial material;
    int armorType;
    int damageReduceAmount;
    int renderIndex;
};
