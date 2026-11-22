#pragma once
#include "Enchantment.h"
class Random;
class EnchantmentDurability : public Enchantment
{
public:
    EnchantmentDurability(int id, int weight);
    int getMinEnchantability(int level) const override;
    int getMaxEnchantability(int level) const override;
    int getMaxLevel() const override;
    bool canApply(ItemStack *stack) const override;
    static bool negateDamage(ItemStack *stack, int level, Random &random);
};
