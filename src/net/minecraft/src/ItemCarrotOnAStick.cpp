#include "ItemCarrotOnAStick.h"
#include "ItemStack.h"
#include "Item.h"
#include "EntityPlayer.h"
ItemStack *ItemCarrotOnAStick::onItemRightClick(ItemStack *stack, World *, EntityPlayer *player) {
    // EntityPig has no controller object in this port yet; retain the Java durability contract.
    if (player != nullptr && stack->getItemDamage() >= stack->getMaxDamage()) { ItemStack *rod=new ItemStack(Item::fishingRod); rod->setItemDamage(stack->getItemDamage()); return rod; }
    return stack;
}
bool ItemCarrotOnAStick::canBoost(ItemStack *stack, EntityPlayer *player) const { return stack && player && remainingUses(stack) > 0 && player->isRiding(); }
int ItemCarrotOnAStick::boostCost(ItemStack *stack) const { return stack ? stack->getMaxDamage() : 0; }
int ItemCarrotOnAStick::remainingUses(ItemStack *stack) const { return stack ? stack->getMaxDamage() - stack->getItemDamageForDisplay() : 0; }
ItemStack *ItemCarrotOnAStick::convertToFishingRod(ItemStack *stack) const { ItemStack *rod = new ItemStack(Item::fishingRod); if (stack) rod->setItemDamage(stack->getItemDamageForDisplay()); return rod; }
ItemStack *ItemCarrotOnAStick::copyWithDamage(ItemStack *stack, int damage) const { if (!stack) return nullptr; ItemStack *copy = stack->copy(); copy->setItemDamage(damage); return copy; }
