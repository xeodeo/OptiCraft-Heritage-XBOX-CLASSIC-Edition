#include "BlockRedstoneRepeater.h"
#include "World.h"
#include "IBlockAccess.h"
#include "Block.h"
#include "Item.h"
#include "EntityPlayer.h"
#include "java/Random.h"

const double BlockRedstoneRepeater::isPowered[4] = {-0.0625, 0.0625, 0.1875, 0.3125};
const int_t BlockRedstoneRepeater::repeaterState[4] = {1, 2, 3, 4};

BlockRedstoneRepeater::BlockRedstoneRepeater(int_t id, bool isPowered)
	: BlockRedstoneLogic(id, isPowered)
{
}

int_t BlockRedstoneRepeater::getBlockTextureFromSideAndMetadata(int_t i, int_t j)
{
	(void)j;
	if (i == 0) return !isRepeaterPowered ? 115 : 99;
	if (i == 1) return !isRepeaterPowered ? 131 : 147;
	return 5;
}

int_t BlockRedstoneRepeater::getBlockTextureFromSide(int_t i)
{
	return getBlockTextureFromSideAndMetadata(i, 0);
}

int_t BlockRedstoneRepeater::getRenderType()
{
	return 36;
}

bool BlockRedstoneRepeater::blockActivated(World *world, int_t i, int_t j, int_t k, EntityPlayer *entityplayer)
{
	(void)entityplayer;
	int_t meta = world->getBlockMetadata(i, j, k);
	int_t delay = (meta & 12) >> 2;
	delay = ((delay + 1) << 2) & 12;
	world->setBlockMetadataWithNotify152(i, j, k, delay | (meta & 3), 3);
	return true;
}

int_t BlockRedstoneRepeater::idDropped(int_t metadata, Random &random)
{
	(void)metadata; (void)random;
	return Item::redstoneRepeater->shiftedIndex;
}

bool BlockRedstoneRepeater::func_94476_e(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k, int_t l)
{
	return func_94482_f(iblockaccess, i, j, k, l) > 0;
}

bool BlockRedstoneRepeater::func_94477_d(int_t blockId)
{
	return isRedstoneRepeaterBlockID(blockId);
}

int_t BlockRedstoneRepeater::func_94481_j_(int_t metadata)
{
	return repeaterState[(metadata & 12) >> 2] * 2;
}

BlockRedstoneLogic *BlockRedstoneRepeater::func_94485_e()
{
	return dynamic_cast<BlockRedstoneLogic *>(Block::redstoneRepeaterActive);
}

BlockRedstoneLogic *BlockRedstoneRepeater::func_94484_i()
{
	return dynamic_cast<BlockRedstoneLogic *>(Block::redstoneRepeaterIdle);
}

void BlockRedstoneRepeater::onBlockRemoval(World *world, int_t i, int_t j, int_t k)
{
	Block::onBlockRemoval(world, i, j, k);
	func_94483_i_(world, i, j, k);
}

void BlockRedstoneRepeater::randomDisplayTick(World *world, int_t i, int_t j, int_t k, Random &random)
{
	if (!isRepeaterPowered) return;
	int_t meta = world->getBlockMetadata(i, j, k);
	int_t dir = getDirection(meta);
	double d = (double)((float)i + 0.5f) + (double)(random.nextFloat() - 0.5f) * 0.2;
	double d1 = (double)((float)j + 0.4f) + (double)(random.nextFloat() - 0.5f) * 0.2;
	double d2 = (double)((float)k + 0.5f) + (double)(random.nextFloat() - 0.5f) * 0.2;
	double d3 = 0.0;
	double d4 = 0.0;
	if (random.nextInt(2) == 0)
	{
		switch (dir)
		{
		case 0: d4 = -0.3125; break;
		case 1: d3 =  0.3125; break;
		case 2: d4 =  0.3125; break;
		case 3: d3 = -0.3125; break;
		}
	}
	else
	{
		int_t delay = (meta & 12) >> 2;
		switch (dir)
		{
		case 0: d4 =  isPowered[delay]; break;
		case 1: d3 = -isPowered[delay]; break;
		case 2: d4 = -isPowered[delay]; break;
		case 3: d3 =  isPowered[delay]; break;
		}
	}
	world->spawnParticle("reddust", d + d3, d1, d2 + d4, 0.0, 0.0, 0.0);
}
