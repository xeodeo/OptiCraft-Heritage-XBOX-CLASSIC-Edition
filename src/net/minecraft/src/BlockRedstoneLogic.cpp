#include "BlockRedstoneLogic.h"
#include "Material.h"
#include "World.h"
#include "IBlockAccess.h"
#include "Direction.h"
#include "MathHelper.h"
#include "EntityLiving.h"
#include <algorithm>

BlockRedstoneLogic::BlockRedstoneLogic(int_t id, bool isPowered)
	: BlockDirectional(id, Material::circuits), isRepeaterPowered(isPowered)
{
	setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 0.125f, 1.0f);
}

bool BlockRedstoneLogic::renderAsNormalBlock()
{
	return false;
}

bool BlockRedstoneLogic::canPlaceBlockAt(World *world, int_t i, int_t j, int_t k)
{
	if (!world->isBlockNormalCube(i, j - 1, k))
		return false;
	return Block::canPlaceBlockAt(world, i, j, k);
}

bool BlockRedstoneLogic::canBlockStay(World *world, int_t i, int_t j, int_t k)
{
	if (!world->isBlockNormalCube(i, j - 1, k))
		return false;
	return Block::canBlockStay(world, i, j, k);
}

void BlockRedstoneLogic::updateTick(World *world, int_t i, int_t j, int_t k, Random &random)
{
	(void)random;
	int_t meta = world->getBlockMetadata(i, j, k);
	if (!func_94476_e(world, i, j, k, meta))
	{
		bool shouldBePowered = func_94478_d(world, i, j, k, meta);
		if (isRepeaterPowered && !shouldBePowered)
		{
			world->setBlockNotify152(i, j, k, func_94484_i()->blockID, meta, 2);
		}
		else if (!isRepeaterPowered)
		{
			world->setBlockNotify152(i, j, k, func_94485_e()->blockID, meta, 2);
			if (!shouldBePowered)
			{
				world->func_82740_a(i, j, k, func_94485_e()->blockID, func_94486_g(meta), -1);
			}
		}
	}
}

bool BlockRedstoneLogic::shouldSideBeRendered(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k, int_t l)
{
	(void)iblockaccess; (void)i; (void)j; (void)k;
	return l != 0 && l != 1;
}

int_t BlockRedstoneLogic::getRenderType()
{
	return 36;
}

bool BlockRedstoneLogic::func_96470_c(int_t metadata)
{
	(void)metadata;
	return isRepeaterPowered;
}

int_t BlockRedstoneLogic::isProvidingStrongPower(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k, int_t l)
{
	return isProvidingWeakPower(iblockaccess, i, j, k, l);
}

int_t BlockRedstoneLogic::isProvidingWeakPower(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k, int_t l)
{
	int_t meta = iblockaccess->getBlockMetadata(i, j, k);
	if (!func_96470_c(meta))
	{
		return 0;
	}
	int_t dir = getDirection(meta);
	if (dir == 0 && l == 3)
		return func_94480_d(iblockaccess, i, j, k, meta);
	if (dir == 1 && l == 4)
		return func_94480_d(iblockaccess, i, j, k, meta);
	if (dir == 2 && l == 2)
		return func_94480_d(iblockaccess, i, j, k, meta);
	if (dir == 3 && l == 5)
		return func_94480_d(iblockaccess, i, j, k, meta);
	return 0;
}

bool BlockRedstoneLogic::isPoweringTo(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k, int_t l)
{
	return isProvidingWeakPower(iblockaccess, i, j, k, l) > 0;
}

bool BlockRedstoneLogic::isIndirectlyPoweringTo(World *world, int_t i, int_t j, int_t k, int_t l)
{
	return isProvidingStrongPower(world, i, j, k, l) > 0;
}

void BlockRedstoneLogic::onNeighborBlockChange(World *world, int_t i, int_t j, int_t k, int_t l)
{
	if (!canBlockStay(world, i, j, k))
	{
		dropBlockAsItem(world, i, j, k, world->getBlockMetadata(i, j, k));
		world->setBlockWithNotify(i, j, k, 0);
		world->notifyBlocksOfNeighborChange(i + 1, j, k, blockID);
		world->notifyBlocksOfNeighborChange(i - 1, j, k, blockID);
		world->notifyBlocksOfNeighborChange(i, j, k + 1, blockID);
		world->notifyBlocksOfNeighborChange(i, j, k - 1, blockID);
		world->notifyBlocksOfNeighborChange(i, j - 1, k, blockID);
		world->notifyBlocksOfNeighborChange(i, j + 1, k, blockID);
	}
	else
	{
		func_94479_f(world, i, j, k, l);
	}
}

void BlockRedstoneLogic::func_94479_f(World *world, int_t i, int_t j, int_t k, int_t l)
{
	(void)l;
	int_t meta = world->getBlockMetadata(i, j, k);
	if (!func_94476_e(world, i, j, k, meta))
	{
		bool shouldBePowered = func_94478_d(world, i, j, k, meta);
		if (((isRepeaterPowered && !shouldBePowered) || (!isRepeaterPowered && shouldBePowered)) &&
			!world->isBlockTickScheduled(i, j, k, blockID))
		{
			int_t priority = -1;
			if (func_83011_d(world, i, j, k, meta))
			{
				priority = -3;
			}
			else if (isRepeaterPowered)
			{
				priority = -2;
			}
			world->func_82740_a(i, j, k, blockID, func_94481_j_(meta), priority);
		}
	}
}

bool BlockRedstoneLogic::func_94476_e(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k, int_t l)
{
	(void)iblockaccess; (void)i; (void)j; (void)k; (void)l;
	return false;
}

bool BlockRedstoneLogic::func_94478_d(World *world, int_t i, int_t j, int_t k, int_t l)
{
	return getInputStrength(world, i, j, k, l) > 0;
}

int_t BlockRedstoneLogic::getInputStrength(World *world, int_t i, int_t j, int_t k, int_t l)
{
	int_t dir = getDirection(l);
	int_t nx = i + Direction::offsetX[dir];
	int_t nz = k + Direction::offsetZ[dir];
	int_t power = world->getIndirectPowerLevelTo(nx, j, nz, Direction::directionToFacing[dir]);
	if (power >= 15)
		return power;
	int_t wirePower = 0;
	if (world->getBlockId(nx, j, nz) == Block::redstoneWire->blockID)
		wirePower = world->getBlockMetadata(nx, j, nz);
	return std::max(power, wirePower);
}

int_t BlockRedstoneLogic::func_94482_f(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k, int_t l)
{
	int_t dir = getDirection(l);
	switch (dir)
	{
	case 0:
	case 2:
		return std::max(func_94488_g(iblockaccess, i - 1, j, k, 4), func_94488_g(iblockaccess, i + 1, j, k, 5));
	case 1:
	case 3:
		return std::max(func_94488_g(iblockaccess, i, j, k + 1, 3), func_94488_g(iblockaccess, i, j, k - 1, 2));
	default:
		return 0;
	}
}

int_t BlockRedstoneLogic::func_94488_g(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k, int_t l)
{
	int_t id = iblockaccess->getBlockId(i, j, k);
	if (func_94477_d(id))
	{
		return id == Block::redstoneWire->blockID ? iblockaccess->getBlockMetadata(i, j, k) : iblockaccess->isBlockProvidingPowerTo(i, j, k, l);
	}
	return 0;
}

bool BlockRedstoneLogic::canProvidePower()
{
	return true;
}

void BlockRedstoneLogic::onBlockPlacedBy(World *world, int_t i, int_t j, int_t k, EntityLiving *entityliving)
{
	int_t dir = ((MathHelper::floor_double((double)(entityliving->rotationYaw * 4.0f / 360.0f) + 0.5) & 3) + 2) % 4;
	world->setBlockMetadataWithNotify152(i, j, k, dir, 3);
	bool powered = func_94478_d(world, i, j, k, dir);
	if (powered)
	{
		world->scheduleBlockUpdate(i, j, k, blockID, 1);
	}
}

void BlockRedstoneLogic::onBlockAdded(World *world, int_t i, int_t j, int_t k)
{
	func_94483_i_(world, i, j, k);
}

void BlockRedstoneLogic::func_94483_i_(World *world, int_t i, int_t j, int_t k)
{
	int_t dir = getDirection(world->getBlockMetadata(i, j, k));
	if (dir == 1)
	{
		world->notifyBlockOfNeighborChange(i + 1, j, k, blockID);
		world->notifyBlocksOfNeighborChange(i + 1, j, k, blockID);
	}
	if (dir == 3)
	{
		world->notifyBlockOfNeighborChange(i - 1, j, k, blockID);
		world->notifyBlocksOfNeighborChange(i - 1, j, k, blockID);
	}
	if (dir == 2)
	{
		world->notifyBlockOfNeighborChange(i, j, k + 1, blockID);
		world->notifyBlocksOfNeighborChange(i, j, k + 1, blockID);
	}
	if (dir == 0)
	{
		world->notifyBlockOfNeighborChange(i, j, k - 1, blockID);
		world->notifyBlocksOfNeighborChange(i, j, k - 1, blockID);
	}
}

void BlockRedstoneLogic::onBlockDestroyedByPlayer(World *world, int_t i, int_t j, int_t k, int_t metadata)
{
	if (isRepeaterPowered)
	{
		world->notifyBlocksOfNeighborChange(i + 1, j, k, blockID);
		world->notifyBlocksOfNeighborChange(i - 1, j, k, blockID);
		world->notifyBlocksOfNeighborChange(i, j, k + 1, blockID);
		world->notifyBlocksOfNeighborChange(i, j, k - 1, blockID);
		world->notifyBlocksOfNeighborChange(i, j - 1, k, blockID);
		world->notifyBlocksOfNeighborChange(i, j + 1, k, blockID);
	}
	Block::onBlockDestroyedByPlayer(world, i, j, k, metadata);
}

bool BlockRedstoneLogic::isOpaqueCube()
{
	return false;
}

bool BlockRedstoneLogic::func_94477_d(int_t blockId)
{
	if (blockId <= 0 || blockId >= Block::BLOCK_REGISTRY_SIZE)
		return false;
	Block *b = Block::blocksList[blockId];
	return b != nullptr && b->canProvidePower();
}

int_t BlockRedstoneLogic::func_94480_d(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k, int_t l)
{
	(void)iblockaccess; (void)i; (void)j; (void)k; (void)l;
	return 15;
}

bool BlockRedstoneLogic::isRedstoneRepeaterBlockID(int_t blockId)
{
	return (Block::redstoneRepeaterIdle != nullptr && blockId == Block::redstoneRepeaterIdle->blockID) ||
	       (Block::redstoneRepeaterActive != nullptr && blockId == Block::redstoneRepeaterActive->blockID) ||
	       (Block::redstoneComparatorIdle != nullptr && blockId == Block::redstoneComparatorIdle->blockID) ||
	       (Block::redstoneComparatorActive != nullptr && blockId == Block::redstoneComparatorActive->blockID);
}

bool BlockRedstoneLogic::func_94487_f(int_t blockId)
{
	return (func_94485_e() != nullptr && blockId == func_94485_e()->blockID) ||
	       (func_94484_i() != nullptr && blockId == func_94484_i()->blockID);
}

bool BlockRedstoneLogic::isAssociatedBlockID(int_t otherBlockId)
{
	return func_94487_f(otherBlockId);
}

bool BlockRedstoneLogic::func_83011_d(World *world, int_t i, int_t j, int_t k, int_t l)
{
	int_t dir = getDirection(l);
	int_t backX = i - Direction::offsetX[dir];
	int_t backZ = k - Direction::offsetZ[dir];
	int_t backId = world->getBlockId(backX, j, backZ);
	if (isRedstoneRepeaterBlockID(backId))
	{
		int_t backMeta = world->getBlockMetadata(backX, j, backZ);
		int_t backDir = getDirection(backMeta);
		return backDir != dir;
	}
	return false;
}

int_t BlockRedstoneLogic::func_94486_g(int_t metadata)
{
	return func_94481_j_(metadata);
}
