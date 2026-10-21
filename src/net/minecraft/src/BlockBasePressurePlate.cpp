#include "BlockBasePressurePlate.h"
#include "AxisAlignedBB.h"
#include "Block.h"
#include "IBlockAccess.h"
#include "Material.h"
#include "World.h"

BlockBasePressurePlate::BlockBasePressurePlate(int_t id, int_t textureIndex, Material *material)
	: Block(id, textureIndex, material)
{
	setTickOnLoad(true);
	float f = 0.0625f;
	setBlockBounds(f, 0.0f, f, 1.0f - f, 0.0625f, 1.0f - f);
}

void BlockBasePressurePlate::setBlockBoundsBasedOnState(IBlockAccess *iblockaccess, int_t x, int_t y, int_t z)
{
	setBoundsFromMeta(iblockaccess->getBlockMetadata(x, y, z));
}

void BlockBasePressurePlate::setBoundsFromMeta(int_t meta)
{
	bool active = getPowerSupply(meta) > 0;
	float f = 0.0625f;
	if (active)
	{
		setBlockBounds(f, 0.0f, f, 1.0f - f, 0.03125f, 1.0f - f);
	}
	else
	{
		setBlockBounds(f, 0.0f, f, 1.0f - f, 0.0625f, 1.0f - f);
	}
}

int_t BlockBasePressurePlate::tickRate()
{
	return 20;
}

AxisAlignedBB *BlockBasePressurePlate::getCollisionBoundingBoxFromPool(World *world, int_t x, int_t y, int_t z)
{
	(void)world; (void)x; (void)y; (void)z;
	return nullptr;
}

bool BlockBasePressurePlate::isOpaqueCube()
{
	return false;
}

bool BlockBasePressurePlate::renderAsNormalBlock()
{
	return false;
}

bool BlockBasePressurePlate::getBlocksMovement(IBlockAccess *iblockaccess, int_t x, int_t y, int_t z)
{
	(void)iblockaccess; (void)x; (void)y; (void)z;
	return true;
}

bool BlockBasePressurePlate::canPlaceBlockAt(World *world, int_t x, int_t y, int_t z)
{
	return world->isBlockNormalCube(x, y - 1, z) ||
	       (Block::fence != nullptr && world->getBlockId(x, y - 1, z) == Block::fence->blockID);
}

void BlockBasePressurePlate::onNeighborBlockChange(World *world, int_t x, int_t y, int_t z, int_t neighborId)
{
	(void)neighborId;
	bool valid = world->isBlockNormalCube(x, y - 1, z) ||
	             (Block::fence != nullptr && world->getBlockId(x, y - 1, z) == Block::fence->blockID);
	if (!valid)
	{
		dropBlockAsItem(world, x, y, z, world->getBlockMetadata(x, y, z));
		world->setBlockWithNotify(x, y, z, 0);
	}
}

void BlockBasePressurePlate::updateTick(World *world, int_t x, int_t y, int_t z, Random &random)
{
	(void)random;
	if (world->multiplayerWorld) return;
	int_t power = getPowerSupply(world->getBlockMetadata(x, y, z));
	if (power > 0)
	{
		setStateIfMobInteractsWithPlate(world, x, y, z, power);
	}
}

void BlockBasePressurePlate::onEntityCollidedWithBlock(World *world, int_t x, int_t y, int_t z, Entity *entity)
{
	(void)entity;
	if (world->multiplayerWorld) return;
	int_t power = getPowerSupply(world->getBlockMetadata(x, y, z));
	if (power == 0)
	{
		setStateIfMobInteractsWithPlate(world, x, y, z, power);
	}
}

void BlockBasePressurePlate::setStateIfMobInteractsWithPlate(World *world, int_t x, int_t y, int_t z, int_t currentPower)
{
	int_t newPower = getPlateState(world, x, y, z);
	bool hadPower = currentPower > 0;
	bool hasPower = newPower > 0;
	if (currentPower != newPower)
	{
		// Flags 2 as in 1.5.2: the plate notifies its neighbours below.
		world->setBlockMetadataWithNotify152(x, y, z, getMetaFromWeight(newPower), 2);
		notifyNeighbors(world, x, y, z);
		world->markBlocksDirty(x, y, z, x, y, z);
	}
	if (!hasPower && hadPower)
	{
		world->playSoundEffect((double)x + 0.5, (double)y + 0.1, (double)z + 0.5, "random.click", 0.3f, 0.5f);
	}
	else if (hasPower && !hadPower)
	{
		world->playSoundEffect((double)x + 0.5, (double)y + 0.1, (double)z + 0.5, "random.click", 0.3f, 0.6f);
	}
	if (hasPower)
	{
		world->scheduleBlockUpdate(x, y, z, blockID, tickRate());
	}
}

AxisAlignedBB *BlockBasePressurePlate::getSensitiveAABB(int_t x, int_t y, int_t z)
{
	float f = 0.125f;
	return AxisAlignedBB::getBoundingBoxFromPool(
		(double)((float)x + f), (double)y, (double)((float)z + f),
		(double)((float)(x + 1) - f), (double)y + 0.25, (double)((float)(z + 1) - f));
}

void BlockBasePressurePlate::notifyNeighbors(World *world, int_t x, int_t y, int_t z)
{
	world->notifyBlocksOfNeighborChange(x, y, z, blockID);
	world->notifyBlocksOfNeighborChange(x, y - 1, z, blockID);
}

void BlockBasePressurePlate::onBlockRemoval(World *world, int_t x, int_t y, int_t z)
{
	if (getPowerSupply(world->getBlockMetadata(x, y, z)) > 0)
	{
		notifyNeighbors(world, x, y, z);
	}
	Block::onBlockRemoval(world, x, y, z);
}

int_t BlockBasePressurePlate::isProvidingWeakPower(IBlockAccess *iblockaccess, int_t x, int_t y, int_t z, int_t side)
{
	(void)side;
	return getPowerSupply(iblockaccess->getBlockMetadata(x, y, z));
}

int_t BlockBasePressurePlate::isProvidingStrongPower(IBlockAccess *iblockaccess, int_t x, int_t y, int_t z, int_t side)
{
	return side == 1 ? getPowerSupply(iblockaccess->getBlockMetadata(x, y, z)) : 0;
}

bool BlockBasePressurePlate::isPoweringTo(IBlockAccess *iblockaccess, int_t x, int_t y, int_t z, int_t side)
{
	return isProvidingWeakPower(iblockaccess, x, y, z, side) > 0;
}

bool BlockBasePressurePlate::isIndirectlyPoweringTo(World *world, int_t x, int_t y, int_t z, int_t side)
{
	return isProvidingStrongPower(world, x, y, z, side) > 0;
}

bool BlockBasePressurePlate::canProvidePower()
{
	return true;
}

void BlockBasePressurePlate::setBlockBoundsForItemRender()
{
	float f = 0.5f;
	float f1 = 0.125f;
	float f2 = 0.5f;
	setBlockBounds(0.5f - f, 0.5f - f1, 0.5f - f2, 0.5f + f, 0.5f + f1, 0.5f + f2);
}

int_t BlockBasePressurePlate::getMobilityFlag()
{
	return 1;
}
