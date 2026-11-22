#include "EnchantmentDamage.h"
#include "EntityLiving.h"
#include "ItemAxe.h"
#include "ItemStack.h"
#include "MathHelper.h"
#include "EnumCreatureAttribute.h"

namespace
{
const char *kDamageNames[] = { "all", "undead", "arthropods" };
const int kBaseEnchantability[] = { 1, 5, 5 };
const int kLevelEnchantability[] = { 11, 8, 8 };
const int kThresholdEnchantability[] = { 20, 20, 20 };
}
EnchantmentDamage::EnchantmentDamage(int id, int weight, int typeValue) : Enchantment(id, weight, EnumEnchantmentType::weapon), damageType(typeValue) {}
int EnchantmentDamage::getMinEnchantability(int level) const { return kBaseEnchantability[damageType] + (level - 1) * kLevelEnchantability[damageType]; }
int EnchantmentDamage::getMaxEnchantability(int level) const { return getMinEnchantability(level) + kThresholdEnchantability[damageType]; }
int EnchantmentDamage::getMaxLevel() const { return 5; }
int EnchantmentDamage::calcModifierLiving(int level, EntityLiving *living) const
{
    if (damageType == 0) return MathHelper::floor_float(static_cast<float>(level) * 2.75f);
    if (living == nullptr) return 0;
    if (damageType == 1 && living->getCreatureAttribute() == EnumCreatureAttribute::UNDEAD) return MathHelper::floor_float(static_cast<float>(level) * 4.5f);
    if (damageType == 2 && living->getCreatureAttribute() == EnumCreatureAttribute::ARTHROPOD) return MathHelper::floor_float(static_cast<float>(level) * 4.5f);
    return 0;
}
std::string EnchantmentDamage::getName() const { return std::string("enchantment.damage.") + kDamageNames[damageType]; }
bool EnchantmentDamage::canApplyTogether(const Enchantment *other) const { return dynamic_cast<const EnchantmentDamage *>(other) == nullptr; }
bool EnchantmentDamage::canApply(ItemStack *stack) const
{
    if (stack != nullptr && dynamic_cast<ItemAxe *>(stack->getItem()) != nullptr)
        return true;
    return Enchantment::canApply(stack);
}
