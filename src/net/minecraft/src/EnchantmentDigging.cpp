#include "EnchantmentDigging.h"
#include "Item.h"
#include "ItemStack.h"
EnchantmentDigging::EnchantmentDigging(int id, int weight) : Enchantment(id, weight, EnumEnchantmentType::digger) { setName("digging"); }
int EnchantmentDigging::getMinEnchantability(int level) const { return 1 + 10 * (level - 1); }
int EnchantmentDigging::getMaxEnchantability(int level) const { return Enchantment::getMinEnchantability(level) + 50; }
int EnchantmentDigging::getMaxLevel() const { return 5; }
bool EnchantmentDigging::canApply(ItemStack *stack) const
{
    if (stack != nullptr && Item::shears != nullptr && stack->itemID == Item::shears->shiftedIndex)
        return true;
    return Enchantment::canApply(stack);
}
