#include "BlockPoweredOre.h"

BlockPoweredOre::BlockPoweredOre(int_t i, int_t j) : BlockOreStorage(i, j)
{
}

bool BlockPoweredOre::canProvidePower()
{
	return true;
}

bool BlockPoweredOre::isPoweringTo(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k, int_t l)
{
	return true;
}
