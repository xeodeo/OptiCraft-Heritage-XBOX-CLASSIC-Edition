#include "EnchantmentThorns.h"

#include "DamageSource.h"
#include "EnchantmentHelper.h"
#include "Entity.h"
#include "EntityLiving.h"
#include "EntityPlayer.h"
#include "InventoryPlayer.h"
#include "ItemArmor.h"
#include "ItemStack.h"
#include "World.h"
#include "java/Random.h"

EnchantmentThorns::EnchantmentThorns(int id, int weight)
    : Enchantment(id, weight, EnumEnchantmentType::armor_torso)
{
    setName("thorns");
}

int EnchantmentThorns::getMinEnchantability(int level) const { return 10 + 20 * (level - 1); }
int EnchantmentThorns::getMaxEnchantability(int level) const { return Enchantment::getMinEnchantability(level) + 50; }
int EnchantmentThorns::getMaxLevel() const { return 3; }

bool EnchantmentThorns::canApply(ItemStack *stack) const
{
    if (stack != nullptr && dynamic_cast<ItemArmor *>(stack->getItem()) != nullptr)
        return true;
    return Enchantment::canApply(stack);
}

bool EnchantmentThorns::func_92094_a(int level, Random &random)
{
    return level <= 0 ? false : random.nextFloat() < 0.15f * static_cast<float>(level);
}

int EnchantmentThorns::func_92095_b(int level, Random &random)
{
    return level > 10 ? level - 10 : 1 + random.nextInt(4);
}

void EnchantmentThorns::func_92096_a(Entity *attacker, EntityLiving *victim, Random &random)
{
    if (attacker == nullptr || victim == nullptr || Enchantment::thorns == nullptr)
        return;

    const int level = EnchantmentHelper::func_92098_i(victim);
    ItemStack *armor = EnchantmentHelper::func_92099_a(Enchantment::thorns, victim);
    if (func_92094_a(level, random))
    {
        attacker->attackEntityFrom(DamageSource::causeThornsDamage(victim), func_92095_b(level, random));
        if (attacker->worldObj != nullptr)
            attacker->worldObj->playSoundAtEntity(attacker, "damage.thorns", 0.5f, 1.0f);
        if (armor != nullptr)
            armor->damageItem(3, victim);
    }
    else if (armor != nullptr)
    {
        armor->damageItem(1, victim);
    }

    // Java leaves a size-0 stack in the armour slot; here the slot owns the
    // pointer, so free a piece that broke the same way damageArmor does.
    EntityPlayer *player = dynamic_cast<EntityPlayer *>(victim);
    if (armor != nullptr && armor->stackSize <= 0 && player != nullptr && player->inventory != nullptr)
    {
        for (int i = 0; i < 4; ++i)
        {
            if (player->inventory->armorInventory[i] == armor)
            {
                armor->onItemDestroyedByUse(player);
                delete armor;
                player->inventory->armorInventory[i] = nullptr;
                break;
            }
        }
    }
}
