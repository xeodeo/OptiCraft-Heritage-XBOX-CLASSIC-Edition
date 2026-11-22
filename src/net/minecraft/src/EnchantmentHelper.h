#pragma once

#include <vector>
#include <map>

#include "EnchantmentData.h"

class DamageSource;
class Enchantment;
class Entity;
class EntityLiving;
class InventoryPlayer;
class ItemStack;
class Random;

class EnchantmentHelper
{
public:
    static int getEnchantmentLevel(int effectId, ItemStack *stack);
    static std::map<int, int> getEnchantments(ItemStack *stack);
    static void setEnchantments(const std::map<int, int> &enchMap, ItemStack *stack);
    static int getEnchantmentModifierDamage(InventoryPlayer *inventory, const DamageSource &source);
    static int getEnchantmentModifierLiving(InventoryPlayer *inventory, EntityLiving *living);
    static int getKnockbackModifier(InventoryPlayer *inventory, EntityLiving *living);
    static int getFireAspectModifier(InventoryPlayer *inventory, EntityLiving *living);
    static int getRespiration(InventoryPlayer *inventory);
    static int getEfficiencyModifier(InventoryPlayer *inventory);
    static int getUnbreakingModifier(InventoryPlayer *inventory);
    static bool getSilkTouchModifier(InventoryPlayer *inventory);
    static int getFortuneModifier(InventoryPlayer *inventory);
    static int getLootingModifier(InventoryPlayer *inventory);
    static bool getAquaAffinityModifier(InventoryPlayer *inventory);
    // Java 1.5.2: highest Thorns level on the entity's worn items, and the first worn item carrying it.
    static int func_92098_i(EntityLiving *living);
    static ItemStack *func_92099_a(Enchantment *enchantment, EntityLiving *living);
    // Java 1.5.2 getMaxEnchantmentLevel(id, entity.getLastActiveItems()).
    static int getMaxEnchantmentLevel(int effectId, Entity *entity);

    static int calcItemStackEnchantability(Random &random, int slot, int bookshelves, ItemStack *stack);
    static void addRandomEnchantment(Random &random, ItemStack *stack, int level);
    static std::vector<EnchantmentData> buildEnchantmentList(Random &random, ItemStack *stack, int level);
    static std::vector<EnchantmentData> mapEnchantmentData(int enchantability, ItemStack *stack);

    // Java getLastActiveItems(): a player's armour; other mobs here only carry a held item.
    static int getLastActiveItems(Entity *entity, ItemStack **out);

private:
    static int getMaxEnchantmentLevel(int effectId, ItemStack **stacks, int count);
};
