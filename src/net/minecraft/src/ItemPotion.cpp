#include "ItemPotion.h"

#include <memory>

#include "EntityPlayer.h"
#include "EntityPotion.h"
#include "InventoryPlayer.h"
#include "ItemStack.h"
#include "NBTTagCompound.h"
#include "NBTTagList.h"
#include "Potion.h"
#include "PotionHelper.h"
#include "StatCollector.h"
#include "World.h"

ItemPotion::ItemPotion(int id) : Item(id)
{
    setMaxStackSize(1);
    setHasSubtypes(true);
    setMaxDamage(0);
}

const std::vector<PotionEffect> &ItemPotion::getEffects(ItemStack *itemstack)
{
    return getEffects(itemstack != nullptr ? itemstack->getItemDamage() : 0);
}

std::vector<PotionEffect> ItemPotion::getEffectsForStack(ItemStack *itemstack)
{
    if (itemstack == nullptr || !itemstack->hasTagCompound())
        return getEffects(itemstack);
    NBTTagCompound *tag = itemstack->getTagCompound();
    if (tag == nullptr || !tag->hasKey("CustomPotionEffects"))
        return getEffects(itemstack);
    std::vector<PotionEffect> effects;
    NBTTagList *list = tag->getTagList("CustomPotionEffects");
    if (list == nullptr)
        return effects;
    for (int i = 0; i < list->tagCount(); ++i)
    {
        NBTTagCompound *effectTag = dynamic_cast<NBTTagCompound *>(list->tagAt(i));
        PotionEffect *effect = PotionEffect::readCustomPotionEffectFromNBT(effectTag);
        if (effect != nullptr)
        {
            effects.push_back(*effect);
            delete effect;
        }
    }
    return effects;
}

bool ItemPotion::hasCustomPotionEffects(ItemStack *itemstack) const
{
    NBTTagCompound *tag = itemstack != nullptr ? itemstack->getTagCompound() : nullptr;
    return tag != nullptr && tag->hasKey("CustomPotionEffects");
}

std::vector<int> ItemPotion::getSubItemDamageValues()
{
    std::vector<int> damages{0};
    std::vector<std::vector<PotionEffect>> representedEffects;
    for (int base = 0; base <= 15; ++base)
    {
        for (int splash = 0; splash <= 1; ++splash)
        {
            const int form = base | (splash == 0 ? 8192 : 16384);
            for (int level = 0; level <= 2; ++level)
            {
                const int damage = form | (level == 1 ? 32 : (level == 2 ? 64 : 0));
                const std::vector<PotionEffect> &effects = getEffects(damage);
                if (effects.empty())
                    continue;
                bool duplicate = false;
                for (const auto &previous : representedEffects)
                {
                    if (previous.size() != effects.size())
                        continue;
                    bool same = true;
                    for (std::size_t i = 0; i < effects.size(); ++i)
                        same = same && previous[i].equals(effects[i]);
                    if (same)
                    {
                        duplicate = true;
                        break;
                    }
                }
                if (!duplicate)
                {
                    representedEffects.push_back(effects);
                    damages.push_back(damage);
                }
            }
        }
    }
    return damages;
}

const std::vector<PotionEffect> &ItemPotion::getEffects(int damage)
{
    auto it = effectCache.find(damage);
    if (it == effectCache.end())
        it = effectCache.emplace(damage, PotionHelper::getPotionEffects(damage, false)).first;
    return it->second;
}

ItemStack *ItemPotion::onFoodEaten(ItemStack *itemstack, World *world, EntityPlayer *player)
{
    if (!player->capabilities.isCreativeMode)
        --itemstack->stackSize;
    if (!world->multiplayerWorld)
    {
        const auto effects = getEffectsForStack(itemstack);
        for (const PotionEffect &effect : effects)
            player->addPotionEffect(new PotionEffect(effect));
    }

    if (!player->capabilities.isCreativeMode && itemstack->stackSize <= 0)
        return new ItemStack(Item::glassBottle);

    if (!player->capabilities.isCreativeMode)
    {
        ItemStack *bottle = new ItemStack(Item::glassBottle);
        if (!player->inventory->addItemStackToInventory(bottle))
            delete bottle;
    }
    return itemstack;
}

int ItemPotion::getMaxItemUseDuration(ItemStack *)
{
    return 32;
}

EnumAction ItemPotion::getItemUseAction(ItemStack *)
{
    return EnumAction::drink;
}

ItemStack *ItemPotion::onItemRightClick(ItemStack *itemstack, World *world, EntityPlayer *player)
{
    if (isSplash(itemstack->getItemDamage()))
    {
        if (!player->capabilities.isCreativeMode)
            --itemstack->stackSize;
        world->playSoundAtEntity(player, "random.bow", 0.5f, 0.4f / (itemRand.nextFloat() * 0.4f + 0.8f));
        if (!world->multiplayerWorld)
        {
            EntityPotion *potion = new EntityPotion(world, player, itemstack);
            if (!world->spawnEntityInWorld(potion))
                delete potion;
        }
        return itemstack;
    }

    player->setItemInUse(itemstack, getMaxItemUseDuration(itemstack));
    return itemstack;
}

bool ItemPotion::onItemUse(ItemStack *, EntityPlayer *, World *, int, int, int, int)
{
    return false;
}

int ItemPotion::getIconFromDamage(int damage)
{
    return isSplash(damage) ? 154 : 140;
}

int ItemPotion::func_46057_a(int damage, int renderPass)
{
    return renderPass == 0 ? 141 : Item::func_46057_a(damage, renderPass);
}

bool ItemPotion::isSplash(int damage)
{
    return (damage & 16384) != 0;
}

int ItemPotion::getColorFromDamage(int damage)
{
    return PotionHelper::func_40358_a(damage, false);
}

int ItemPotion::getColorFromDamage(int damage, int renderPass)
{
    return renderPass > 0 ? 0xffffff : PotionHelper::func_40358_a(damage, false);
}

bool ItemPotion::func_46058_c()
{
    return true;
}

bool ItemPotion::isEffectInstant(int damage)
{
    const auto &effects = getEffects(damage);
    for (const PotionEffect &effect : effects)
    {
        Potion *potion = Potion::getPotion(effect.getPotionID());
        if (potion != nullptr && potion->isInstant())
            return true;
    }
    return false;
}

bool ItemPotion::hasEffect(ItemStack *itemstack)
{
    return itemstack != nullptr && !getEffectsForStack(itemstack).empty();
}

std::string ItemPotion::getItemDisplayName(ItemStack *itemstack)
{
    if (itemstack == nullptr || itemstack->getItemDamage() == 0)
        return StatCollector::translateToLocal("item.emptyPotion.name");

    std::string prefix;
    if (isSplash(itemstack->getItemDamage()))
        prefix = StatCollector::translateToLocal("potion.prefix.grenade") + " ";

    const std::vector<PotionEffect> effects = getEffectsForStack(itemstack);
    if (!effects.empty())
        return prefix + StatCollector::translateToLocal(effects.front().getEffectName() + ".postfix");

    return StatCollector::translateToLocal(PotionHelper::func_40359_b(itemstack->getItemDamage())) +
           " " + Item::getItemDisplayName(itemstack);
}

void ItemPotion::addInformation(ItemStack *itemstack, std::vector<std::string>& information)
{
    if (itemstack == nullptr || itemstack->getItemDamage() == 0)
        return;

    const std::vector<PotionEffect> effects = getEffectsForStack(itemstack);
    if (effects.empty())
    {
        information.push_back(u8"§7" + StatCollector::translateToLocal("potion.empty"));
        return;
    }

    Potion::initPotions();
    for (const PotionEffect& effect : effects)
    {
        std::string line = StatCollector::translateToLocal(effect.getEffectName());
        if (effect.getAmplifier() > 0)
            line += " " + StatCollector::translateToLocal("potion.potency." + std::to_string(effect.getAmplifier()));
        if (effect.getDuration() > 20)
            line += " (" + Potion::getDurationString(effect) + ")";

        Potion *potion = Potion::getPotion(effect.getPotionID());
        information.push_back(std::string(potion != nullptr && potion->getIsBadEffect() ? u8"§c" : u8"§7") + line);
    }
}

static bool areEffectsEqual(const std::vector<PotionEffect> &a, const std::vector<PotionEffect> &b)
{
    if (a.size() != b.size())
        return false;
    for (size_t i = 0; i < a.size(); ++i)
    {
        if (!a[i].equals(b[i]))
            return false;
    }
    return true;
}

void ItemPotion::getSubItems(int id, CreativeTabs *tab, std::vector<ItemStack *> &list)
{
    Item::getSubItems(id, tab, list);

    static std::vector<int> cachedDamageValues;
    if (cachedDamageValues.empty())
    {
        std::vector<std::vector<PotionEffect>> seenEffects;
        for (int var4 = 0; var4 <= 15; ++var4)
        {
            for (int var5 = 0; var5 <= 1; ++var5)
            {
                int var6 = (var5 == 0) ? (var4 | 8192) : (var4 | 16384);
                for (int var7 = 0; var7 <= 2; ++var7)
                {
                    int var8 = var6;
                    if (var7 == 1)
                        var8 = var6 | 32;
                    else if (var7 == 2)
                        var8 = var6 | 64;

                    std::vector<PotionEffect> effects = PotionHelper::getPotionEffects(var8, false);
                    if (!effects.empty())
                    {
                        bool found = false;
                        for (size_t i = 0; i < seenEffects.size(); ++i)
                        {
                            if (areEffectsEqual(seenEffects[i], effects))
                            {
                                cachedDamageValues[i] = var8;
                                found = true;
                                break;
                            }
                        }
                        if (!found)
                        {
                            seenEffects.push_back(effects);
                            cachedDamageValues.push_back(var8);
                        }
                    }
                }
            }
        }
    }

    for (int dmg : cachedDamageValues)
    {
        list.push_back(new ItemStack(id, 1, dmg));
    }
}
