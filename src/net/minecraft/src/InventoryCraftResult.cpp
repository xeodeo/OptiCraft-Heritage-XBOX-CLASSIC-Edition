#include "InventoryCraftResult.h"
#include "ItemStack.h"

InventoryCraftResult::InventoryCraftResult() {
    stackResult = new ItemStack*[1]();
    stackResult[0] = nullptr;
}

InventoryCraftResult::~InventoryCraftResult() {
    delete stackResult[0];
    delete[] stackResult;
}

int_t InventoryCraftResult::getSizeInventory() {
    return 1;
}

// As in Java the index is ignored: the anvil's result slot is index 2.
ItemStack* InventoryCraftResult::getStackInSlot(int_t) {
    return stackResult[0];
}

std::string InventoryCraftResult::getInvName() {
    return "Result";
}


ItemStack *InventoryCraftResult::getStackInSlotOnClosing(int_t i)
{
    (void)i;
    if (stackResult[0] == nullptr)
        return nullptr;
    ItemStack *stack = stackResult[0];
    stackResult[0] = nullptr;
    return stack;
}

ItemStack* InventoryCraftResult::decrStackSize(int_t, int_t amount) {
    if (amount > 0 && stackResult[0] != nullptr) {
        ItemStack* stack = stackResult[0];
        stackResult[0] = nullptr;
        return stack;
    }
    return nullptr;
}

void InventoryCraftResult::setInventorySlotContents(int_t, ItemStack* stack) {
    if (stackResult[0] != stack)
        delete stackResult[0];
    stackResult[0] = stack;
}

int_t InventoryCraftResult::getInventoryStackLimit() {
    return 64;
}

void InventoryCraftResult::onInventoryChanged() {
}

bool InventoryCraftResult::canInteractWith(EntityPlayer* player) {
    return true;
}
