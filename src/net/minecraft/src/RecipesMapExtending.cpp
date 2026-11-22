#include "RecipesMapExtending.h"
#include "InventoryCrafting.h"
#include "ItemStack.h"
#include "Item.h"
#include "NBTTagCompound.h"
#include "ItemMap.h"
#include "MapData.h"

static ItemStack **createRecipePattern()
{
    ItemStack **items = new ItemStack*[9];
    items[0] = new ItemStack(Item::paper);
    items[1] = new ItemStack(Item::paper);
    items[2] = new ItemStack(Item::paper);
    items[3] = new ItemStack(Item::paper);
    items[4] = new ItemStack(Item::mapItem, 0, -1); // ShapedRecipes uses -1 as the wildcard
    items[5] = new ItemStack(Item::paper);
    items[6] = new ItemStack(Item::paper);
    items[7] = new ItemStack(Item::paper);
    items[8] = new ItemStack(Item::paper);
    return items;
}

RecipesMapExtending::RecipesMapExtending()
    : ShapedRecipes(3, 3, createRecipePattern(), new ItemStack(Item::emptyMap, 0, 0))
{
}

bool RecipesMapExtending::matches(InventoryCrafting *inv)
{
    return matches(inv, nullptr);
}

bool RecipesMapExtending::matches(InventoryCrafting *inv, World *world)
{
    if (!ShapedRecipes::matches(inv))
    {
        return false;
    }
    if (inv == nullptr)
    {
        return false;
    }

    ItemStack *mapStack = nullptr;
    for (int i = 0; i < inv->getSizeInventory() && mapStack == nullptr; i++)
    {
        ItemStack *stack = inv->getStackInSlot(i);
        if (stack != nullptr && Item::mapItem != nullptr && stack->itemID == Item::mapItem->shiftedIndex)
        {
            mapStack = stack;
        }
    }

    if (mapStack == nullptr)
    {
        return false;
    }

    // Java 1.5.2: a map already at the widest zoom (scale 4) cannot be extended.
    if (world == nullptr)
    {
        return false;
    }
    MapData *mapData = static_cast<ItemMap *>(Item::mapItem)->getMapData(mapStack, world);
    return mapData == nullptr ? false : mapData->scale < 4;
}

ItemStack *RecipesMapExtending::getCraftingResult(InventoryCrafting *inv)
{
    if (inv == nullptr)
    {
        return nullptr;
    }

    ItemStack *mapStack = nullptr;
    for (int i = 0; i < inv->getSizeInventory() && mapStack == nullptr; i++)
    {
        ItemStack *stack = inv->getStackInSlot(i);
        if (stack != nullptr && Item::mapItem != nullptr && stack->itemID == Item::mapItem->shiftedIndex)
        {
            mapStack = stack;
        }
    }

    if (mapStack == nullptr)
    {
        return nullptr;
    }

    ItemStack *result = mapStack->copy();
    result->stackSize = 1;
    if (result->getTagCompound() == nullptr)
    {
        result->setTagCompound(new NBTTagCompound());
    }
    result->getTagCompound()->setBoolean("map_is_scaling", true);
    return result;
}
