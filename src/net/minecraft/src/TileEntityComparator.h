#pragma once

#include "TileEntity.h"

class NBTTagCompound;

// net.minecraft.src.TileEntityComparator
class TileEntityComparator : public TileEntity
{
private:
	int_t outputSignal = 0;

public:
	TileEntityComparator();

	void writeToNBT(NBTTagCompound *nbttagcompound) override;
	void readFromNBT(NBTTagCompound *nbttagcompound) override;

	int_t func_96100_a() const;
	void func_96099_a(int_t signal);
};
