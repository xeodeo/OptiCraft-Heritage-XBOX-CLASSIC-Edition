#include "RecipeFireworks.h"

#include <vector>

#include "InventoryCrafting.h"
#include "Item.h"
#include "ItemDye.h"
#include "ItemStack.h"
#include "MathHelper.h"
#include "NBTTagCompound.h"
#include "NBTTagList.h"

std::unique_ptr<ItemStack> RecipeFireworks::buildResult(InventoryCrafting *inv) const
{
    if (inv == nullptr || Item::firework == nullptr || Item::fireworkCharge == nullptr)
        return nullptr;

    int paperCount = 0;
    int gunpowderCount = 0;
    int dyeCount = 0;
    int starCount = 0;
    int effectModCount = 0;
    int shapeModCount = 0;

    const int_t size = inv->getSizeInventory();
    for (int_t i = 0; i < size; ++i)
    {
        ItemStack *slot = inv->getStackInSlot(i);
        if (slot == nullptr)
            continue;

        if (slot->itemID == Item::gunpowder->shiftedIndex)
            ++gunpowderCount;
        else if (slot->itemID == Item::fireworkCharge->shiftedIndex)
            ++starCount;
        else if (slot->itemID == Item::dyePowder->shiftedIndex)
            ++dyeCount;
        else if (slot->itemID == Item::paper->shiftedIndex)
            ++paperCount;
        else if (slot->itemID == Item::lightStoneDust->shiftedIndex)
            ++effectModCount;
        else if (slot->itemID == Item::diamond->shiftedIndex)
            ++effectModCount;
        else if (slot->itemID == Item::fireballCharge->shiftedIndex)
            ++shapeModCount;
        else if (slot->itemID == Item::feather->shiftedIndex)
            ++shapeModCount;
        else if (slot->itemID == Item::goldNugget->shiftedIndex)
            ++shapeModCount;
        else if (Item::skull != nullptr && slot->itemID == Item::skull->shiftedIndex)
            ++shapeModCount;
        else
            return nullptr;
    }

    const int starExtrasCount = effectModCount + dyeCount + shapeModCount;
    if (gunpowderCount > 3 || paperCount > 1)
        return nullptr;

    // 1. Firework Rocket: 1 paper + 1..3 gunpowder + 0..N firework stars
    if (gunpowderCount >= 1 && paperCount == 1 && starExtrasCount == 0)
    {
        std::unique_ptr<ItemStack> result(new ItemStack(Item::firework));
        NBTTagCompound *root = new NBTTagCompound();
        NBTTagCompound *fireworks = new NBTTagCompound("Fireworks");
        if (starCount > 0)
        {
            NBTTagList *explosions = new NBTTagList("Explosions");
            for (int_t i = 0; i < size; ++i)
            {
                ItemStack *slot = inv->getStackInSlot(i);
                if (slot != nullptr &&
                    slot->itemID == Item::fireworkCharge->shiftedIndex &&
                    slot->hasTagCompound() &&
                    slot->getTagCompound()->hasKey("Explosion"))
                {
                    NBTTagCompound *srcExplosion = slot->getTagCompound()->getCompoundTag("Explosion");
                    if (srcExplosion != nullptr)
                        explosions->appendTag(srcExplosion->copy());
                }
            }
            fireworks->setTag("Explosions", explosions);
        }
        fireworks->setByte("Flight", static_cast<byte_t>(gunpowderCount));
        root->setTag("Fireworks", fireworks);
        result->setTagCompound(root);
        return result;
    }

    // 2. Firework Star: 1 gunpowder + 1..N dyes + optional <=1 shape modifier + optional twinkle/trail
    if (gunpowderCount == 1 && paperCount == 0 && starCount == 0 && dyeCount > 0 && shapeModCount <= 1)
    {
        std::unique_ptr<ItemStack> result(new ItemStack(Item::fireworkCharge));
        NBTTagCompound *root = new NBTTagCompound();
        NBTTagCompound *explosion = new NBTTagCompound("Explosion");
        byte_t shape = 0;
        std::vector<int_t> colors;

        for (int_t i = 0; i < size; ++i)
        {
            ItemStack *slot = inv->getStackInSlot(i);
            if (slot == nullptr)
                continue;

            if (slot->itemID == Item::dyePowder->shiftedIndex)
            {
                const int_t dyeIdx = MathHelper::clamp_int(slot->getItemDamage(), 0, 15);
                colors.push_back(ItemDye::worldChunkManager[dyeIdx]);
            }
            else if (slot->itemID == Item::lightStoneDust->shiftedIndex)
            {
                explosion->setBoolean("Flicker", true);
            }
            else if (slot->itemID == Item::diamond->shiftedIndex)
            {
                explosion->setBoolean("Trail", true);
            }
            else if (slot->itemID == Item::fireballCharge->shiftedIndex)
            {
                shape = 1;
            }
            else if (slot->itemID == Item::feather->shiftedIndex)
            {
                shape = 4;
            }
            else if (slot->itemID == Item::goldNugget->shiftedIndex)
            {
                shape = 2;
            }
            else if (Item::skull != nullptr && slot->itemID == Item::skull->shiftedIndex)
            {
                shape = 3;
            }
        }

        explosion->setIntArray("Colors", colors);
        explosion->setByte("Type", shape);
        root->setTag("Explosion", explosion);
        result->setTagCompound(root);
        return result;
    }

    // 3. Firework Star Fade: 1 existing firework star + 1..N dyes
    if (gunpowderCount == 0 && paperCount == 0 && starCount == 1 && dyeCount > 0 && dyeCount == starExtrasCount)
    {
        std::vector<int_t> fadeColors;
        std::unique_ptr<ItemStack> result;

        for (int_t i = 0; i < size; ++i)
        {
            ItemStack *slot = inv->getStackInSlot(i);
            if (slot == nullptr)
                continue;

            if (slot->itemID == Item::dyePowder->shiftedIndex)
            {
                const int_t dyeIdx = MathHelper::clamp_int(slot->getItemDamage(), 0, 15);
                fadeColors.push_back(ItemDye::worldChunkManager[dyeIdx]);
            }
            else if (slot->itemID == Item::fireworkCharge->shiftedIndex)
            {
                result.reset(slot->copy());
                result->stackSize = 1;
            }
        }

        if (result != nullptr && result->hasTagCompound() && result->getTagCompound()->hasKey("Explosion"))
        {
            NBTTagCompound *explosion = result->getTagCompound()->getCompoundTag("Explosion");
            if (explosion != nullptr)
            {
                explosion->setIntArray("FadeColors", fadeColors);
                return result;
            }
        }
        return nullptr;
    }

    return nullptr;
}

bool RecipeFireworks::matches(InventoryCrafting *inventorycrafting)
{
    cachedResult = buildResult(inventorycrafting);
    return cachedResult != nullptr;
}

ItemStack *RecipeFireworks::getCraftingResult(InventoryCrafting *inventorycrafting)
{
    if (cachedResult != nullptr)
        return cachedResult->copy();
    std::unique_ptr<ItemStack> fresh = buildResult(inventorycrafting);
    return fresh != nullptr ? fresh.release() : nullptr;
}

int_t RecipeFireworks::getRecipeSize()
{
    return 10;
}

ItemStack *RecipeFireworks::getRecipeOutput()
{
    return cachedResult.get();
}
