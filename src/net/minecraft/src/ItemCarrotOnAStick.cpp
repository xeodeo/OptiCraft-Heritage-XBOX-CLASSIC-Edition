#include "ItemCarrotOnAStick.h"
#include "ItemStack.h"
#include "Item.h"
#include "EntityPlayer.h"
ItemStack *ItemCarrotOnAStick::onItemRightClick(ItemStack *stack, World *, EntityPlayer *player) {
    // EntityPig has no controller object in this port yet; retain the Java durability contract.
    if (player != nullptr && stack->getItemDamage() >= stack->getMaxDamage()) { ItemStack *rod=new ItemStack(Item::fishingRod); rod->setItemDamage(stack->getItemDamage()); return rod; }
    return stack;
}
