#include "EnchantmentDurability.h"
#include "ItemArmor.h"
#include "ItemStack.h"
#include "java/Random.h"
EnchantmentDurability::EnchantmentDurability(int id, int weight) : Enchantment(id, weight, EnumEnchantmentType::digger) { setName("durability"); }
int EnchantmentDurability::getMinEnchantability(int level) const { return 5 + (level - 1) * 8; }
int EnchantmentDurability::getMaxEnchantability(int level) const { return Enchantment::getMinEnchantability(level) + 50; }
int EnchantmentDurability::getMaxLevel() const { return 3; }
bool EnchantmentDurability::canApply(ItemStack *stack) const
{
    if (stack != nullptr && stack->isItemStackDamageable())
        return true;
    return Enchantment::canApply(stack);
}
bool EnchantmentDurability::negateDamage(ItemStack *stack, int level, Random &random)
{
    if (stack != nullptr && dynamic_cast<ItemArmor *>(stack->getItem()) != nullptr && random.nextFloat() < 0.6f)
        return false;
    return random.nextInt(level + 1) > 0;
}
