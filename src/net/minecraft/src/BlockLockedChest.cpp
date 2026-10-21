#include "BlockLockedChest.h"
#include "Material.h"
#include "IBlockAccess.h"
#include "World.h"
#include "Block.h"

BlockLockedChest::BlockLockedChest(int_t i) : Block(i, Material::wood)
{
	blockIndexInTexture = 4; // planks
}

// 1.5.2 gives the locked chest no icon of its own (it removes itself on its
// first tick); planks keeps it drawable without the old chest cells.
int_t BlockLockedChest::getBlockTexture(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k, int_t l)
{
	(void)iblockaccess; (void)i; (void)j; (void)k; (void)l;
	return blockIndexInTexture;
}

int_t BlockLockedChest::getBlockTextureFromSide(int_t i)
{
	(void)i;
	return blockIndexInTexture;
}

bool BlockLockedChest::canPlaceBlockAt(World *world, int_t i, int_t j, int_t k)
{
	return true;
}

void BlockLockedChest::updateTick(World *world, int_t i, int_t j, int_t k, Random &random)
{
	world->setBlockWithNotify(i, j, k, 0);
}
