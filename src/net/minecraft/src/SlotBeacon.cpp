#include "SlotBeacon.h"

#include "ContainerBeacon.h"
#include "Item.h"
#include "ItemStack.h"

SlotBeacon::SlotBeacon(ContainerBeacon *container, IInventory *inv, int index, int x, int y)
    : Slot(inv, index, x, y),
      beacon(container)
{
}

bool SlotBeacon::isItemValid(ItemStack *stack)
{
    if (stack == nullptr)
        return false;

    return (Item::emerald != nullptr && stack->itemID == Item::emerald->itemID) ||
           (Item::diamond != nullptr && stack->itemID == Item::diamond->itemID) ||
           (Item::ingotGold != nullptr && stack->itemID == Item::ingotGold->itemID) ||
           (Item::ingotIron != nullptr && stack->itemID == Item::ingotIron->itemID);
}

int SlotBeacon::getSlotStackLimit()
{
    return 1;
}
