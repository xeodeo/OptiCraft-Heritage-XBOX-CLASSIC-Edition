#include "TileEntityComparator.h"
#include "NBTTagCompound.h"

TileEntityComparator::TileEntityComparator()
	: outputSignal(0)
{
}

void TileEntityComparator::writeToNBT(NBTTagCompound *nbttagcompound)
{
	TileEntity::writeToNBT(nbttagcompound);
	nbttagcompound->setInteger("OutputSignal", outputSignal);
}

void TileEntityComparator::readFromNBT(NBTTagCompound *nbttagcompound)
{
	TileEntity::readFromNBT(nbttagcompound);
	outputSignal = nbttagcompound->getInteger("OutputSignal");
}

int_t TileEntityComparator::func_96100_a() const
{
	return outputSignal;
}

void TileEntityComparator::func_96099_a(int_t signal)
{
	outputSignal = signal;
}
