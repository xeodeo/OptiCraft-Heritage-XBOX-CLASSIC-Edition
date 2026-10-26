#pragma once
#include "InventoryBasic.h"
class ContainerRepair;
class InventoryRepair final : public InventoryBasic {
public:
    explicit InventoryRepair(ContainerRepair *owner) : InventoryBasic("Repair",2), owner(owner) {}
private:
    ContainerRepair *owner;
};
