#include "RecipesMapCloning.h"
#include "InventoryCrafting.h"
#include "ItemStack.h"
#include "Item.h"

bool RecipesMapCloning::matches(InventoryCrafting *inv)
{
    if (inv == nullptr)
    {
        return false;
    }

    int emptyMapCount = 0;
    ItemStack *sourceMap = nullptr;

    for (int i = 0; i < inv->getSizeInventory(); i++)
    {
        ItemStack *slotStack = inv->getStackInSlot(i);
        if (slotStack != nullptr)
        {
            if (Item::mapItem != nullptr && slotStack->itemID == Item::mapItem->shiftedIndex)
            {
                if (sourceMap != nullptr)
                {
                    return false;
                }
                sourceMap = slotStack;
            }
            else
            {
                if (Item::emptyMap == nullptr || slotStack->itemID != Item::emptyMap->shiftedIndex)
                {
                    return false;
                }
                emptyMapCount++;
            }
        }
    }

    return sourceMap != nullptr && emptyMapCount > 0;
}

ItemStack *RecipesMapCloning::getCraftingResult(InventoryCrafting *inv)
{
    if (inv == nullptr)
    {
        return nullptr;
    }

    int emptyMapCount = 0;
    ItemStack *sourceMap = nullptr;

    for (int i = 0; i < inv->getSizeInventory(); i++)
    {
        ItemStack *slotStack = inv->getStackInSlot(i);
        if (slotStack != nullptr)
        {
            if (Item::mapItem != nullptr && slotStack->itemID == Item::mapItem->shiftedIndex)
            {
                if (sourceMap != nullptr)
                {
                    return nullptr;
                }
                sourceMap = slotStack;
            }
            else
            {
                if (Item::emptyMap == nullptr || slotStack->itemID != Item::emptyMap->shiftedIndex)
                {
                    return nullptr;
                }
                emptyMapCount++;
            }
        }
    }

    if (sourceMap != nullptr && emptyMapCount >= 1)
    {
        ItemStack *result = new ItemStack(Item::mapItem, emptyMapCount + 1, sourceMap->getItemDamage());
        if (sourceMap->hasDisplayName())
        {
            result->setItemName(sourceMap->getDisplayName());
        }
        return result;
    }

    return nullptr;
}

int_t RecipesMapCloning::getRecipeSize()
{
    return 9;
}

ItemStack *RecipesMapCloning::getRecipeOutput()
{
    return nullptr;
}
