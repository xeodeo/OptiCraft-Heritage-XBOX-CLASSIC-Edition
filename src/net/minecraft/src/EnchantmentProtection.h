#pragma once
#include "Enchantment.h"
class Entity;
class EnchantmentProtection : public Enchantment
{
public:
    EnchantmentProtection(int id, int weight, int protectionType);
    int getMinEnchantability(int level) const override;
    int getMaxEnchantability(int level) const override;
    int getMaxLevel() const override;
    int calcModifierDamage(int level, const DamageSource &source) const override;
    std::string getName() const override;
    bool canApplyTogether(const Enchantment *other) const override;
    // Java 1.5.2: fire protection shortens burning, blast protection cuts explosion knockback.
    static int func_92093_a(Entity *entity, int fireTicks);
    static double func_92092_a(Entity *entity, double knockback);
    const int protectionType;
};
