#include "EnchantmentProtection.h"
#include "DamageSource.h"
#include "EnchantmentHelper.h"
#include "Entity.h"
#include "MathHelper.h"

namespace
{
const char *kProtectionNames[] = { "all", "fire", "fall", "explosion", "projectile" };
const int kBaseEnchantability[] = { 1, 10, 5, 5, 3 };
const int kLevelEnchantability[] = { 11, 8, 6, 8, 6 };
const int kThresholdEnchantability[] = { 20, 12, 10, 12, 15 };
}

EnchantmentProtection::EnchantmentProtection(int id, int weight, int typeValue)
    : Enchantment(id, weight, EnumEnchantmentType::armor), protectionType(typeValue)
{
    if (typeValue == 2)
        type = EnumEnchantmentType::armor_feet;
}
int EnchantmentProtection::getMinEnchantability(int level) const { return kBaseEnchantability[protectionType] + (level - 1) * kLevelEnchantability[protectionType]; }
int EnchantmentProtection::getMaxEnchantability(int level) const { return getMinEnchantability(level) + kThresholdEnchantability[protectionType]; }
int EnchantmentProtection::getMaxLevel() const { return 4; }
int EnchantmentProtection::calcModifierDamage(int level, const DamageSource &source) const
{
    if (source.canHarmInCreative()) return 0;
    const float modifier = static_cast<float>(6 + level * level) / 3.0f;
    if (protectionType == 0) return MathHelper::floor_float(modifier * 0.75f);
    if (protectionType == 1 && source.isFireDamage()) return MathHelper::floor_float(modifier * 1.25f);
    if (protectionType == 2 && &source == &DamageSource::fall) return MathHelper::floor_float(modifier * 2.5f);
    if (protectionType == 3 && source.isExplosion()) return MathHelper::floor_float(modifier * 1.5f);
    if (protectionType == 4 && source.isProjectile()) return MathHelper::floor_float(modifier * 1.5f);
    return 0;
}
std::string EnchantmentProtection::getName() const { return std::string("enchantment.protect.") + kProtectionNames[protectionType]; }
bool EnchantmentProtection::canApplyTogether(const Enchantment *other) const
{
    const EnchantmentProtection *protection = dynamic_cast<const EnchantmentProtection *>(other);
    if (protection == nullptr) return Enchantment::canApplyTogether(other);
    if (protection->protectionType == protectionType) return false;
    return protectionType == 2 || protection->protectionType == 2;
}

int EnchantmentProtection::func_92093_a(Entity *entity, int fireTicks)
{
    if (Enchantment::fireProtection == nullptr)
        return fireTicks;
    const int level = EnchantmentHelper::getMaxEnchantmentLevel(Enchantment::fireProtection->effectId, entity);
    if (level > 0)
        fireTicks -= MathHelper::floor_float(static_cast<float>(fireTicks) * static_cast<float>(level) * 0.15f);
    return fireTicks;
}

double EnchantmentProtection::func_92092_a(Entity *entity, double knockback)
{
    if (Enchantment::blastProtection == nullptr)
        return knockback;
    const int level = EnchantmentHelper::getMaxEnchantmentLevel(Enchantment::blastProtection->effectId, entity);
    if (level > 0)
        knockback -= static_cast<double>(MathHelper::floor_double(knockback * static_cast<double>(static_cast<float>(level) * 0.15f)));
    return knockback;
}
