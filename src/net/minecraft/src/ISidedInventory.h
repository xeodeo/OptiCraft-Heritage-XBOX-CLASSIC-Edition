#pragma once

#include <vector>
#include "IInventory.h"

class ItemStack;

// net.minecraft.src.ISidedInventory
class ISidedInventory : public virtual IInventory
{
public:
	virtual ~ISidedInventory() = default;

	virtual std::vector<int_t> getAccessibleSlotsFromSide(int_t side) = 0;
	virtual bool canInsertItem(int_t slot, ItemStack *stack, int_t side) = 0;
	virtual bool canExtractItem(int_t slot, ItemStack *stack, int_t side) = 0;
};
