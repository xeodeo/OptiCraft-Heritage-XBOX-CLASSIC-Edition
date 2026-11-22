#include "ItemBook.h"

#include "ItemStack.h"

ItemBook::ItemBook(int id)
    : Item(id)
{
}

bool ItemBook::isItemTool(ItemStack *stack)
{
    return stack != nullptr && stack->stackSize == 1;
}

int ItemBook::getItemEnchantability()
{
    return 1;
}
