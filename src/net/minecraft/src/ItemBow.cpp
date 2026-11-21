#include "ItemBow.h"

#include "Enchantment.h"
#include "EnchantmentHelper.h"
#include "EntityArrow.h"
#include "EntityPlayer.h"
#include "InventoryPlayer.h"
#include "ItemStack.h"
#include "World.h"

ItemBow::ItemBow(int id) : Item(id)
{
    maxStackSize = 1;
    setMaxDamage(384);
}

bool ItemBow::hasArrow(EntityPlayer* entityplayer)
{
    if (entityplayer == nullptr || entityplayer->inventory == nullptr)
        return false;
    if (entityplayer->capabilities.isCreativeMode)
        return true;
    if (Item::arrow == nullptr)
        return false;
    return entityplayer->inventory->getInventorySlotContainItem(Item::arrow->shiftedIndex) >= 0;
}

float ItemBow::getLaunchPower(int charge)
{
    float p = static_cast<float>(charge) / 20.0f;
    p = (p * p + p * 2.0f) / 3.0f;
    return (p > 1.0f) ? 1.0f : p;
}

ItemStack* ItemBow::onFoodEaten(ItemStack* itemstack, World*, EntityPlayer*)
{
    return itemstack;
}

void ItemBow::onPlayerStoppedUsing(ItemStack* itemstack, World* world, EntityPlayer* entityplayer, int timeLeft)
{
    if (itemstack == nullptr || world == nullptr || entityplayer == nullptr || entityplayer->inventory == nullptr)
        return;

    const bool infinite = entityplayer->capabilities.isCreativeMode ||
        (Enchantment::infinity != nullptr && EnchantmentHelper::getEnchantmentLevel(Enchantment::infinity->effectId, itemstack) > 0);
    if (!infinite && !hasArrow(entityplayer))
        return;

    const int charge = getMaxItemUseDuration(itemstack) - timeLeft;
    const float strength = getLaunchPower(charge);
    if (strength < 0.1f)
        return;

    EntityArrow* arrow = new EntityArrow(world, entityplayer, strength * 2.0f);
    if (strength >= 1.0f)
        arrow->arrowCritical = true;

    if (Enchantment::power != nullptr)
    {
        const int power = EnchantmentHelper::getEnchantmentLevel(Enchantment::power->effectId, itemstack);
        if (power > 0)
            arrow->setDamage(arrow->getDamage() + static_cast<double>(power) * 0.5 + 0.5);
    }
    if (Enchantment::punch != nullptr)
    {
        const int punch = EnchantmentHelper::getEnchantmentLevel(Enchantment::punch->effectId, itemstack);
        if (punch > 0)
            arrow->setKnockbackStrength(punch);
    }
    if (Enchantment::flame != nullptr && EnchantmentHelper::getEnchantmentLevel(Enchantment::flame->effectId, itemstack) > 0)
        arrow->setFire(100);

    itemstack->damageItem(1, entityplayer);
    world->playSoundAtEntity(entityplayer, "random.bow", 1.0f,
        1.0f / (itemRand.nextFloat() * 0.4f + 1.2f) + strength * 0.5f);

    if (!infinite)
    {
        if (Item::arrow != nullptr)
            entityplayer->inventory->consumeInventoryItem(Item::arrow->shiftedIndex);
    }
    else
    {
        arrow->doesArrowBelongToPlayer = false;
    }

    if (!world->multiplayerWorld)
    {
        if (!world->entityJoinedWorld(arrow))
            delete arrow;
    }
    else
    {
        delete arrow;
    }
}

int ItemBow::getMaxItemUseDuration(ItemStack*) { return 72000; }
EnumAction ItemBow::getItemUseAction(ItemStack*) { return EnumAction::bow; }

ItemStack* ItemBow::onItemRightClick(ItemStack* itemstack, World*, EntityPlayer* entityplayer)
{
    if (hasArrow(entityplayer))
        entityplayer->setItemInUse(itemstack, getMaxItemUseDuration(itemstack));
    return itemstack;
}

int ItemBow::getItemEnchantability() { return 1; }
