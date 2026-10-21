#include "BlockRedstoneWire.h"

#include "IBlockAccess.h"
#include "Item.h"
#include "Material.h"
#include "Direction.h"
#include "World.h"
#include "java/Random.h"

BlockRedstoneWire::BlockRedstoneWire(int_t i, int_t j) :
	Block(i, j, Material::circuits),
	wiresProvidePower(true)
{
	setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 0.0625f, 1.0f);
}

int_t BlockRedstoneWire::getBlockTextureFromSideAndMetadata(int_t i, int_t j)
{
	return blockIndexInTexture;
}

AxisAlignedBB *BlockRedstoneWire::getCollisionBoundingBoxFromPool(World *world, int_t i, int_t j, int_t k)
{
	return nullptr;
}

bool BlockRedstoneWire::isOpaqueCube()
{
	return false;
}

bool BlockRedstoneWire::renderAsNormalBlock()
{
	return false;
}

int_t BlockRedstoneWire::getRenderType()
{
	return 5;
}

int_t BlockRedstoneWire::colorMultiplier(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k)
{
	return 0x800000;
}

bool BlockRedstoneWire::canPlaceBlockAt(World *world, int_t i, int_t j, int_t k)
{
	return world->isBlockNormalCube(i, j - 1, k) || world->getBlockId(i, j - 1, k) == Block::glowStone->blockID;
}

void BlockRedstoneWire::updateAndPropagateCurrentStrength(World *world, int_t i, int_t j, int_t k)
{
	calculateCurrentChanges(world, i, j, k, i, j, k);
	std::vector<ChunkPosition> arraylist = pendingWireUpdates.valuesInIterationOrder();
	pendingWireUpdates.clear();
	for (const ChunkPosition &position : arraylist)
	{
		world->notifyBlocksOfNeighborChange(position.x, position.y, position.z, blockID);
	}
}

void BlockRedstoneWire::calculateCurrentChanges(World *world, int_t i, int_t j, int_t k, int_t l, int_t i1, int_t j1)
{
	int_t currentMeta = world->getBlockMetadata(i, j, k);
	int_t targetStrength = 0;
	targetStrength = getMaxCurrentStrength(world, l, i1, j1, targetStrength);

	wiresProvidePower = false;
	int_t strongestPower = world->getStrongestIndirectPower(i, j, k);
	wiresProvidePower = true;

	if (strongestPower > 0 && strongestPower > targetStrength - 1)
	{
		targetStrength = strongestPower;
	}

	int_t neighborMaxWire = 0;
	for (int_t step = 0; step < 4; ++step)
	{
		int_t nx = i;
		int_t nz = k;
		if (step == 0) --nx;
		if (step == 1) ++nx;
		if (step == 2) --nz;
		if (step == 3) ++nz;

		if (nx != l || nz != j1)
		{
			neighborMaxWire = getMaxCurrentStrength(world, nx, j, nz, neighborMaxWire);
		}

		if (world->isBlockNormalCube(nx, j, nz) && !world->isBlockNormalCube(i, j + 1, k))
		{
			if ((nx != l || nz != j1) && j >= i1)
			{
				neighborMaxWire = getMaxCurrentStrength(world, nx, j + 1, nz, neighborMaxWire);
			}
		}
		else if (!world->isBlockNormalCube(nx, j, nz) && (nx != l || nz != j1) && j <= i1)
		{
			neighborMaxWire = getMaxCurrentStrength(world, nx, j - 1, nz, neighborMaxWire);
		}
	}

	if (neighborMaxWire > targetStrength)
	{
		targetStrength = neighborMaxWire - 1;
	}
	else if (targetStrength > 0)
	{
		--targetStrength;
	}
	else
	{
		targetStrength = 0;
	}

	if (strongestPower > targetStrength - 1)
	{
		targetStrength = strongestPower;
	}

	if (currentMeta != targetStrength)
	{
		world->setBlockMetadataWithNotify(i, j, k, targetStrength);
		pendingWireUpdates.add(ChunkPosition(i, j, k));
		pendingWireUpdates.add(ChunkPosition(i - 1, j, k));
		pendingWireUpdates.add(ChunkPosition(i + 1, j, k));
		pendingWireUpdates.add(ChunkPosition(i, j - 1, k));
		pendingWireUpdates.add(ChunkPosition(i, j + 1, k));
		pendingWireUpdates.add(ChunkPosition(i, j, k - 1));
		pendingWireUpdates.add(ChunkPosition(i, j, k + 1));
	}
}

void BlockRedstoneWire::notifyWireNeighborsOfNeighborChange(World *world, int_t i, int_t j, int_t k)
{
	if (world->getBlockId(i, j, k) != blockID)
	{
		return;
	}
	else
	{
		world->notifyBlocksOfNeighborChange(i, j, k, blockID);
		world->notifyBlocksOfNeighborChange(i - 1, j, k, blockID);
		world->notifyBlocksOfNeighborChange(i + 1, j, k, blockID);
		world->notifyBlocksOfNeighborChange(i, j, k - 1, blockID);
		world->notifyBlocksOfNeighborChange(i, j, k + 1, blockID);
		world->notifyBlocksOfNeighborChange(i, j - 1, k, blockID);
		world->notifyBlocksOfNeighborChange(i, j + 1, k, blockID);
		return;
	}
}

void BlockRedstoneWire::onBlockAdded(World *world, int_t i, int_t j, int_t k)
{
	Block::onBlockAdded(world, i, j, k);
	if (world->multiplayerWorld)
	{
		return;
	}
	updateAndPropagateCurrentStrength(world, i, j, k);
	world->notifyBlocksOfNeighborChange(i, j + 1, k, blockID);
	world->notifyBlocksOfNeighborChange(i, j - 1, k, blockID);
	notifyWireNeighborsOfNeighborChange(world, i - 1, j, k);
	notifyWireNeighborsOfNeighborChange(world, i + 1, j, k);
	notifyWireNeighborsOfNeighborChange(world, i, j, k - 1);
	notifyWireNeighborsOfNeighborChange(world, i, j, k + 1);
	if (world->isBlockNormalCube(i - 1, j, k)) notifyWireNeighborsOfNeighborChange(world, i - 1, j + 1, k);
	else notifyWireNeighborsOfNeighborChange(world, i - 1, j - 1, k);
	if (world->isBlockNormalCube(i + 1, j, k)) notifyWireNeighborsOfNeighborChange(world, i + 1, j + 1, k);
	else notifyWireNeighborsOfNeighborChange(world, i + 1, j - 1, k);
	if (world->isBlockNormalCube(i, j, k - 1)) notifyWireNeighborsOfNeighborChange(world, i, j + 1, k - 1);
	else notifyWireNeighborsOfNeighborChange(world, i, j - 1, k - 1);
	if (world->isBlockNormalCube(i, j, k + 1)) notifyWireNeighborsOfNeighborChange(world, i, j + 1, k + 1);
	else notifyWireNeighborsOfNeighborChange(world, i, j - 1, k + 1);
}

void BlockRedstoneWire::onBlockRemoval(World *world, int_t i, int_t j, int_t k)
{
	Block::onBlockRemoval(world, i, j, k);
	if (world->multiplayerWorld)
	{
		return;
	}
	world->notifyBlocksOfNeighborChange(i, j + 1, k, blockID);
	world->notifyBlocksOfNeighborChange(i, j - 1, k, blockID);
	world->notifyBlocksOfNeighborChange(i + 1, j, k, blockID);
	world->notifyBlocksOfNeighborChange(i - 1, j, k, blockID);
	world->notifyBlocksOfNeighborChange(i, j, k + 1, blockID);
	world->notifyBlocksOfNeighborChange(i, j, k - 1, blockID);
	updateAndPropagateCurrentStrength(world, i, j, k);
	notifyWireNeighborsOfNeighborChange(world, i - 1, j, k);
	notifyWireNeighborsOfNeighborChange(world, i + 1, j, k);
	notifyWireNeighborsOfNeighborChange(world, i, j, k - 1);
	notifyWireNeighborsOfNeighborChange(world, i, j, k + 1);
	if (world->isBlockNormalCube(i - 1, j, k)) notifyWireNeighborsOfNeighborChange(world, i - 1, j + 1, k);
	else notifyWireNeighborsOfNeighborChange(world, i - 1, j - 1, k);
	if (world->isBlockNormalCube(i + 1, j, k)) notifyWireNeighborsOfNeighborChange(world, i + 1, j + 1, k);
	else notifyWireNeighborsOfNeighborChange(world, i + 1, j - 1, k);
	if (world->isBlockNormalCube(i, j, k - 1)) notifyWireNeighborsOfNeighborChange(world, i, j + 1, k - 1);
	else notifyWireNeighborsOfNeighborChange(world, i, j - 1, k - 1);
	if (world->isBlockNormalCube(i, j, k + 1)) notifyWireNeighborsOfNeighborChange(world, i, j + 1, k + 1);
	else notifyWireNeighborsOfNeighborChange(world, i, j - 1, k + 1);
}

int_t BlockRedstoneWire::getMaxCurrentStrength(World *world, int_t i, int_t j, int_t k, int_t l)
{
	if (world->getBlockId(i, j, k) != blockID)
	{
		return l;
	}
	int_t i1 = world->getBlockMetadata(i, j, k);
	if (i1 > l)
	{
		return i1;
	}
	else
	{
		return l;
	}
}

void BlockRedstoneWire::onNeighborBlockChange(World *world, int_t i, int_t j, int_t k, int_t l)
{
	if (world->multiplayerWorld)
	{
		return;
	}
	int_t i1 = world->getBlockMetadata(i, j, k);
	bool flag = canPlaceBlockAt(world, i, j, k);
	if (!flag)
	{
		dropBlockAsItem(world, i, j, k, i1);
		world->setBlockWithNotify(i, j, k, 0);
	}
	else
	{
		updateAndPropagateCurrentStrength(world, i, j, k);
	}
	Block::onNeighborBlockChange(world, i, j, k, l);
}

int_t BlockRedstoneWire::idDropped(int_t i, Random &random)
{
	return Item::redstone->shiftedIndex;
}

int_t BlockRedstoneWire::isProvidingStrongPower(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k, int_t l)
{
	return !wiresProvidePower ? 0 : isProvidingWeakPower(iblockaccess, i, j, k, l);
}

int_t BlockRedstoneWire::isProvidingWeakPower(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k, int_t l)
{
	if (!wiresProvidePower)
	{
		return 0;
	}
	int_t strength = iblockaccess->getBlockMetadata(i, j, k);
	if (strength == 0)
	{
		return 0;
	}
	if (l == 1)
	{
		return strength;
	}

	bool w = isPoweredOrRepeater(iblockaccess, i - 1, j, k, 1) || (!iblockaccess->isBlockNormalCube(i - 1, j, k) && isPoweredOrRepeater(iblockaccess, i - 1, j - 1, k, -1));
	bool e = isPoweredOrRepeater(iblockaccess, i + 1, j, k, 3) || (!iblockaccess->isBlockNormalCube(i + 1, j, k) && isPoweredOrRepeater(iblockaccess, i + 1, j - 1, k, -1));
	bool n = isPoweredOrRepeater(iblockaccess, i, j, k - 1, 2) || (!iblockaccess->isBlockNormalCube(i, j, k - 1) && isPoweredOrRepeater(iblockaccess, i, j - 1, k - 1, -1));
	bool s = isPoweredOrRepeater(iblockaccess, i, j, k + 1, 0) || (!iblockaccess->isBlockNormalCube(i, j, k + 1) && isPoweredOrRepeater(iblockaccess, i, j - 1, k + 1, -1));

	if (!iblockaccess->isBlockNormalCube(i, j + 1, k))
	{
		if (iblockaccess->isBlockNormalCube(i - 1, j, k) && isPoweredOrRepeater(iblockaccess, i - 1, j + 1, k, -1)) w = true;
		if (iblockaccess->isBlockNormalCube(i + 1, j, k) && isPoweredOrRepeater(iblockaccess, i + 1, j + 1, k, -1)) e = true;
		if (iblockaccess->isBlockNormalCube(i, j, k - 1) && isPoweredOrRepeater(iblockaccess, i, j + 1, k - 1, -1)) n = true;
		if (iblockaccess->isBlockNormalCube(i, j, k + 1) && isPoweredOrRepeater(iblockaccess, i, j + 1, k + 1, -1)) s = true;
	}

	if (!n && !e && !w && !s && l >= 2 && l <= 5)
	{
		return strength;
	}
	if (l == 2 && n && !w && !e)
	{
		return strength;
	}
	if (l == 3 && s && !w && !e)
	{
		return strength;
	}
	if (l == 4 && w && !n && !s)
	{
		return strength;
	}
	if (l == 5 && e && !n && !s)
	{
		return strength;
	}
	return 0;
}

bool BlockRedstoneWire::isPoweringTo(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k, int_t l)
{
	return isProvidingWeakPower(iblockaccess, i, j, k, l) > 0;
}

bool BlockRedstoneWire::isIndirectlyPoweringTo(World *world, int_t i, int_t j, int_t k, int_t l)
{
	return isProvidingStrongPower(world, i, j, k, l) > 0;
}

bool BlockRedstoneWire::canProvidePower()
{
	return wiresProvidePower;
}

void BlockRedstoneWire::randomDisplayTick(World *world, int_t i, int_t j, int_t k, Random &random)
{
	int_t l = world->getBlockMetadata(i, j, k);
	if (l > 0)
	{
		double d = (double)i + 0.5 + ((double)random.nextFloat() - 0.5) * 0.20000000000000001;
		double d1 = (float)j + 0.0625f;
		double d2 = (double)k + 0.5 + ((double)random.nextFloat() - 0.5) * 0.20000000000000001;
		float f = (float)l / 15.0f;
		float f1 = f * 0.6f + 0.4f;
		if (l == 0)
		{
			f1 = 0.0f;
		}
		float f2 = f * f * 0.7f - 0.5f;
		float f3 = f * f * 0.6f - 0.7f;
		if (f2 < 0.0f) f2 = 0.0f;
		if (f3 < 0.0f) f3 = 0.0f;
		world->spawnParticle("reddust", d, d1, d2, f1, f2, f3);
	}
}

bool BlockRedstoneWire::isPowerProviderOrWire(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k, int_t l)
{
	int_t i1 = iblockaccess->getBlockId(i, j, k);
	if (i1 == Block::redstoneWire->blockID)
	{
		return true;
	}
	if (i1 == 0)
	{
		return false;
	}
	if (i1 != Block::redstoneRepeaterIdle->blockID && i1 != Block::redstoneRepeaterActive->blockID)
	{
		return Block::blocksList[i1]->canProvidePower() && l != -1;
	}

	const int_t metadata = iblockaccess->getBlockMetadata(i, j, k) & 3;
	return l == metadata || l == Direction::rotateOpposite[metadata];
}

bool BlockRedstoneWire::isPoweredOrRepeater(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k, int_t l)
{
	if (isPowerProviderOrWire(iblockaccess, i, j, k, l))
	{
		return true;
	}
	int_t i1 = iblockaccess->getBlockId(i, j, k);
	if (i1 == Block::redstoneRepeaterActive->blockID)
	{
		const int_t metadata = iblockaccess->getBlockMetadata(i, j, k) & 3;
		return l == metadata;
	}
	return false;
}
