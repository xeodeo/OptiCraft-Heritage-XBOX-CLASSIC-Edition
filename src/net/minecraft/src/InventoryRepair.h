#pragma once

#include "InventoryBasic.h"
#include "ContainerRepair.h"

class InventoryRepair : public InventoryBasic
{
public:
    InventoryRepair(ContainerRepair *container, const std::string &title, bool isCustom, int slotCount)
        : InventoryBasic(title, slotCount), theContainer(container)
    {
    }

    void onInventoryChanged() override
    {
        InventoryBasic::onInventoryChanged();
        if (theContainer != nullptr)
        {
            theContainer->onCraftMatrixChanged(this);
        }
    }

    bool isStackValidForSlot(int slot, ItemStack *stack) override
    {
        (void)slot;
        (void)stack;
        return true;
    }

private:
    ContainerRepair *theContainer = nullptr;
};
