#include "EnchantmentUntouching.h"
#include "Item.h"
#include "ItemStack.h"
EnchantmentUntouching::EnchantmentUntouching(int id, int weight) : Enchantment(id, weight, EnumEnchantmentType::digger) { setName("untouching"); }
int EnchantmentUntouching::getMinEnchantability(int) const { return 15; }
int EnchantmentUntouching::getMaxEnchantability(int level) const { return Enchantment::getMinEnchantability(level) + 50; }
bool EnchantmentUntouching::canApplyTogether(const Enchantment *other) const { return Enchantment::canApplyTogether(other) && (fortune == nullptr || other == nullptr || other->effectId != fortune->effectId); }
int EnchantmentUntouching::getMaxLevel() const { return 1; }
bool EnchantmentUntouching::canApply(ItemStack *stack) const
{
    if (stack != nullptr && Item::shears != nullptr && stack->itemID == Item::shears->shiftedIndex)
        return true;
    return Enchantment::canApply(stack);
}
