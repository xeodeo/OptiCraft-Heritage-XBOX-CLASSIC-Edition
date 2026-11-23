#include "FurnaceRecipes.h"

#include "Block.h"
#include "Item.h"
#include "ItemStack.h"
#include "mods/ModManager.h"

FurnaceRecipes* FurnaceRecipes::smeltingBase = nullptr;

FurnaceRecipes::FurnaceRecipes()
{
    addSmelting(Block::oreIron->blockID,         new ItemStack(Item::ingotIron),         0.7f);
    addSmelting(Block::oreGold->blockID,         new ItemStack(Item::ingotGold),         1.0f);
    addSmelting(Block::oreDiamond->blockID,      new ItemStack(Item::diamond),           1.0f);
    addSmelting(Block::sand->blockID,            new ItemStack(Block::glass),            0.1f);
    addSmelting(Item::porkRaw->shiftedIndex,     new ItemStack(Item::porkCooked),        0.35f);
    addSmelting(Item::beefRaw->shiftedIndex,     new ItemStack(Item::beefCooked),        0.35f);
    addSmelting(Item::chickenRaw->shiftedIndex,  new ItemStack(Item::chickenCooked),     0.35f);
    addSmelting(Item::fishRaw->shiftedIndex,     new ItemStack(Item::fishCooked),        0.35f);
    addSmelting(Block::cobblestone->blockID,     new ItemStack(Block::stone),            0.1f);
    addSmelting(Item::clay->shiftedIndex,        new ItemStack(Item::brick),             0.3f);
    addSmelting(Block::cactus->blockID,          new ItemStack(Item::dyePowder, 1, 2),   0.2f);
    addSmelting(Block::wood->blockID,            new ItemStack(Item::coal, 1, 1),        0.15f);
    addSmelting(Item::potato->shiftedIndex,      new ItemStack(Item::bakedPotato),       0.35f);
    addSmelting(Block::netherrack->blockID,      new ItemStack(Item::netherrackBrick),   0.1f);
    addSmelting(Block::oreCoal->blockID,         new ItemStack(Item::coal),              0.1f);
    addSmelting(Block::oreRedstone->blockID,     new ItemStack(Item::redstone),          0.7f);
    addSmelting(Block::oreLapis->blockID,        new ItemStack(Item::dyePowder, 1, 4),   0.2f);
    addSmelting(Block::oreEmerald->blockID,      new ItemStack(Item::emerald),           1.0f);
    addSmelting(Block::oreNetherQuartz->blockID, new ItemStack(Item::netherQuartz),      0.2f);
}

FurnaceRecipes* FurnaceRecipes::smelting()
{
    if (smeltingBase == nullptr)
        smeltingBase = new FurnaceRecipes();
    return smeltingBase;
}

void FurnaceRecipes::addSmelting(int_t itemId, ItemStack *output, float experience)
{
    smeltingOrder.add(itemId);
    smeltingList[itemId] = output;
    if (output != nullptr)
        experienceList[output->itemID] = experience;
}

float FurnaceRecipes::getExperience(int_t itemId)
{
    auto it = experienceList.find(itemId);
    return it != experienceList.end() ? it->second : 0.0f;
}

ItemStack* FurnaceRecipes::getSmeltingResult(int_t itemId)
{
    if (!ModManager::isItemAllowed(itemId))
        return nullptr;
    auto it = smeltingList.find(itemId);
    if (it != smeltingList.end())
    {
        if (it->second != nullptr && !ModManager::isItemAllowed(it->second->itemID))
            return nullptr;
        return it->second;
    }
    return nullptr;
}

std::map<int_t, ItemStack*>& FurnaceRecipes::getSmeltingList()
{
    return smeltingList;
}

std::vector<ItemStack*> FurnaceRecipes::getSmeltingValuesInJavaOrder() const
{
    std::vector<ItemStack*> values;
    values.reserve(smeltingList.size());
    for (int_t itemId : smeltingOrder.valuesInIterationOrder())
    {
        if (!ModManager::isItemAllowed(itemId))
            continue;
        auto it = smeltingList.find(itemId);
        if (it != smeltingList.end() && it->second != nullptr && ModManager::isItemAllowed(it->second->itemID))
            values.push_back(it->second);
    }
    return values;
}
