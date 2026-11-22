#pragma once
#include "Enchantment.h"

class Entity;
class EntityLiving;
class Random;

// net.minecraft.src.EnchantmentThorns (Java 1.5.2)
class EnchantmentThorns : public Enchantment
{
public:
    EnchantmentThorns(int id, int weight);
    int getMinEnchantability(int level) const override;
    int getMaxEnchantability(int level) const override;
    int getMaxLevel() const override;
    bool canApply(ItemStack *stack) const override;

    static bool func_92094_a(int level, Random &random);
    static int func_92095_b(int level, Random &random);
    // Called after an attack lands: the victim's thorns armour hurts the attacker.
    static void func_92096_a(Entity *attacker, EntityLiving *victim, Random &random);
};
