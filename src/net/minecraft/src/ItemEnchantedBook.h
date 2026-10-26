#pragma once

#include <vector>
#include "Item.h"
#include "WeightedRandomChestContent.h"

class Random;
class Enchantment;
class EnchantmentData;
class NBTTagList;

class ItemEnchantedBook : public Item
{
public:
    explicit ItemEnchantedBook(int id);

    bool hasEffect(ItemStack *stack) override;
    bool isItemTool(ItemStack *stack) override;
    const EnumRarity &getRarity(ItemStack *stack) override;
    void addInformation(ItemStack *stack, std::vector<std::string> &information) override;

    NBTTagList *func_92110_g(ItemStack *stack);
    void func_92115_a(ItemStack *stack, const EnchantmentData &data);
    ItemStack *func_92111_a(const EnchantmentData &data);
    void func_92113_a(Enchantment *enchantment, std::vector<ItemStack *> &list);
    ItemStack *func_92109_a(Random &random);
    WeightedRandomChestContent func_92114_b(Random &random);
    WeightedRandomChestContent func_92112_a(Random &random, int minCount, int maxCount, int weight);
};
