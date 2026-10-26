#pragma once
#include "Item.h"
class NBTTagList;
class EnchantmentData;
class ItemEnchantedBook : public Item {
public:
    explicit ItemEnchantedBook(int id) : Item(id) { setMaxStackSize(1); }
    bool hasEffect(ItemStack*) override;
    bool isItemTool(ItemStack*) override;
    const EnumRarity& getRarity(ItemStack*) override;
    void addInformation(ItemStack*,std::vector<std::string>&) override;
    NBTTagList *func_92110_g(ItemStack*);
    void func_92115_a(ItemStack*,EnchantmentData*);
    ItemStack *func_92111_a(EnchantmentData*);
    int storedCount(ItemStack*);
    int storedId(ItemStack*,int);
    int storedLevel(ItemStack*,int);
    bool hasStoredEnchantment(ItemStack*,int);
    int enchantmentIdAt(ItemStack*,int);
    int enchantmentLevelAt(ItemStack*,int);
    bool mergeEnchantment(ItemStack*,int,int);
    bool validStoredTag(ItemStack*);
    void clearStoredEnchantments(ItemStack*);
    ItemStack *copyWithEnchantment(ItemStack*,int,int);
    bool canApplyToItem(ItemStack*,int);
    int clampLevel(int level) const;
    void setStoredLevel(ItemStack*,int,int);
    std::string storedDescription(ItemStack*);
    bool storedIsCompatible(ItemStack*,int);
    std::string enchantmentSummary(ItemStack*);
    bool hasAnyStored(ItemStack*);
    int firstStoredId(ItemStack*);
    int firstStoredLevel(ItemStack*);
};
