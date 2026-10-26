#pragma once
#include "Item.h"
class NBTTagList;
class EnchantmentData;
class ItemEnchantedBook : public Item { public: explicit ItemEnchantedBook(int id) : Item(id) { setMaxStackSize(1); } bool hasEffect(ItemStack*) override; bool isItemTool(ItemStack*) override; const EnumRarity& getRarity(ItemStack*) override; NBTTagList *func_92110_g(ItemStack*); void func_92115_a(ItemStack*,EnchantmentData*); ItemStack *func_92111_a(EnchantmentData*); };
