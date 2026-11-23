#pragma once

#include "Item.h"
#include "EnumArmorMaterial.h"

class ItemArmor : public Item {
public:
    ItemArmor(int id, EnumArmorMaterial material, int renderIndex, int armorType);

    static const int maxDamageArray[4];

    int getItemEnchantability() override;
    // Mod armour (ChocoCraft's Chocodisguise): own texture for armour layer 1 or 2
    // instead of /armor/<material>_<layer>.png. nullptr = the vanilla texture.
    virtual const char *getCustomArmorTexture(int layer) const { (void)layer; return nullptr; }

    EnumArmorMaterial material;
    int armorType;
    int damageReduceAmount;
    int renderIndex;
};
