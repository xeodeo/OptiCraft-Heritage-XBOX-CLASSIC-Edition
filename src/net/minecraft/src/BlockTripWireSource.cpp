#include "BlockTripWireSource.h"
#include "AxisAlignedBB.h"
#include "Direction.h"
#include "IBlockAccess.h"
#include "Material.h"
#include "java/Random.h"
#include "Texture152Atlas.h"
#include "World.h"

BlockTripWireSource::BlockTripWireSource(int_t id)
	: Block(id, Material::circuits)
{
	blockIndexInTexture = TEX152_BLOCK_ICON("tripWireSource.png");
	setTickOnLoad(true);
}

AxisAlignedBB *BlockTripWireSource::getCollisionBoundingBoxFromPool(World *world, int_t x, int_t y, int_t z)
{
	(void)world; (void)x; (void)y; (void)z;
	return nullptr;
}

bool BlockTripWireSource::isOpaqueCube()
{
	return false;
}

bool BlockTripWireSource::renderAsNormalBlock()
{
	return false;
}

int_t BlockTripWireSource::getRenderType()
{
	return 29;
}

int_t BlockTripWireSource::tickRate()
{
	return 10;
}

bool BlockTripWireSource::canPlaceBlockOnSide(World *world, int_t x, int_t y, int_t z, int_t side)
{
	if (side == 2 && world->isBlockNormalCube(x, y, z + 1)) return true;
	if (side == 3 && world->isBlockNormalCube(x, y, z - 1)) return true;
	if (side == 4 && world->isBlockNormalCube(x + 1, y, z)) return true;
	if (side == 5 && world->isBlockNormalCube(x - 1, y, z)) return true;
	return false;
}

bool BlockTripWireSource::canPlaceBlockAt(World *world, int_t x, int_t y, int_t z)
{
	if (world->isBlockNormalCube(x - 1, y, z)) return true;
	if (world->isBlockNormalCube(x + 1, y, z)) return true;
	if (world->isBlockNormalCube(x, y, z - 1)) return true;
	if (world->isBlockNormalCube(x, y, z + 1)) return true;
	return false;
}

void BlockTripWireSource::onBlockPlaced(World *world, int_t x, int_t y, int_t z, int_t side)
{
	int_t dir = 0;
	if (side == 2 && world->isBlockNormalCube(x, y, z + 1)) dir = 2;
	if (side == 3 && world->isBlockNormalCube(x, y, z - 1)) dir = 0;
	if (side == 4 && world->isBlockNormalCube(x + 1, y, z)) dir = 1;
	if (side == 5 && world->isBlockNormalCube(x - 1, y, z)) dir = 3;
	world->setBlockMetadataWithNotify(x, y, z, dir);
	func_72143_a(world, x, y, z, blockID, dir, false, -1, 0);
}

void BlockTripWireSource::onBlockAdded(World *world, int_t x, int_t y, int_t z)
{
	func_72143_a(world, x, y, z, blockID, world->getBlockMetadata(x, y, z), false, -1, 0);
}

bool BlockTripWireSource::checkCanStay(World *world, int_t x, int_t y, int_t z)
{
	if (!canPlaceBlockAt(world, x, y, z))
	{
		dropBlockAsItem(world, x, y, z, world->getBlockMetadata(x, y, z));
		world->setBlockWithNotify(x, y, z, 0);
		return false;
	}
	return true;
}

void BlockTripWireSource::onNeighborBlockChange(World *world, int_t x, int_t y, int_t z, int_t neighborId)
{
	if (neighborId != blockID)
	{
		if (checkCanStay(world, x, y, z))
		{
			int_t meta = world->getBlockMetadata(x, y, z);
			int_t dir = meta & 3;
			bool drop = false;
			if (!world->isBlockNormalCube(x - 1, y, z) && dir == 3) drop = true;
			if (!world->isBlockNormalCube(x + 1, y, z) && dir == 1) drop = true;
			if (!world->isBlockNormalCube(x, y, z - 1) && dir == 0) drop = true;
			if (!world->isBlockNormalCube(x, y, z + 1) && dir == 2) drop = true;
			if (drop)
			{
				dropBlockAsItem(world, x, y, z, meta);
				world->setBlockWithNotify(x, y, z, 0);
			}
		}
	}
}

void BlockTripWireSource::func_72143_a(World *world, int_t x, int_t y, int_t z, int_t bId, int_t meta, bool notify, int_t changedDistance, int_t changedMeta)
{
	int_t dir = meta & 3;
	bool wasConnected = (meta & 4) == 4;
	bool wasPowered = (meta & 8) == 8;
	bool isSource = (Block::tripWireSource != nullptr && bId == Block::tripWireSource->blockID);
	bool isValidLine = false;
	bool isSuspended = !world->isBlockNormalCube(x, y - 1, z);
	int_t dx = Direction::offsetX[dir];
	int_t dz = Direction::offsetZ[dir];
	int_t otherHookDist = 0;
	int_t wireMetas[42];

	for (int_t dist = 1; dist < 42; ++dist)
	{
		int_t curX = x + dx * dist;
		int_t curZ = z + dz * dist;
		int_t curBlockId = world->getBlockId(curX, y, curZ);
		if (Block::tripWireSource != nullptr && curBlockId == Block::tripWireSource->blockID)
		{
			int_t otherMeta = world->getBlockMetadata(curX, y, curZ);
			if ((otherMeta & 3) == Direction::rotateOpposite[dir])
			{
				otherHookDist = dist;
			}
			break;
		}

		if (Block::tripWire != nullptr && curBlockId != Block::tripWire->blockID && dist != changedDistance)
		{
			wireMetas[dist] = -1;
			isSource = false;
		}
		else
		{
			int_t wMeta = (dist == changedDistance) ? changedMeta : world->getBlockMetadata(curX, y, curZ);
			bool armed = (wMeta & 8) != 8;
			bool tripped = (wMeta & 1) == 1;
			bool wSuspended = (wMeta & 2) == 2;
			isSource &= (wSuspended == isSuspended);
			isValidLine |= (armed && tripped);
			wireMetas[dist] = wMeta;
			if (dist == changedDistance)
			{
				world->scheduleBlockUpdate(x, y, z, bId, tickRate());
				isSource &= armed;
			}
		}
	}

	isSource &= (otherHookDist > 1);
	isValidLine &= isSource;
	int_t newFlags = (isSource ? 4 : 0) | (isValidLine ? 8 : 0);
	meta = dir | newFlags;
	if (otherHookDist > 0)
	{
		int_t otherX = x + dx * otherHookDist;
		int_t otherZ = z + dz * otherHookDist;
		int_t oppDir = Direction::rotateOpposite[dir];
		world->setBlockMetadataWithNotify(otherX, y, otherZ, oppDir | newFlags);
		notifyNeighborOfChange(world, otherX, y, otherZ, oppDir);
		playSoundEffect(world, otherX, y, otherZ, isSource, isValidLine, wasConnected, wasPowered);
	}

	playSoundEffect(world, x, y, z, isSource, isValidLine, wasConnected, wasPowered);
	if (bId > 0)
	{
		world->setBlockMetadataWithNotify(x, y, z, meta);
		if (notify)
		{
			notifyNeighborOfChange(world, x, y, z, dir);
		}
	}

	if (wasConnected != isSource)
	{
		for (int_t dist = 1; dist < otherHookDist; ++dist)
		{
			int_t curX = x + dx * dist;
			int_t curZ = z + dz * dist;
			int_t wMeta = wireMetas[dist];
			if (wMeta >= 0)
			{
				if (isSource) wMeta |= 4;
				else wMeta &= ~4;
				world->setBlockMetadataWithNotify(curX, y, curZ, wMeta);
			}
		}
	}
}

void BlockTripWireSource::playSoundEffect(World *world, int_t x, int_t y, int_t z, bool connected, bool powered, bool wasConnected, bool wasPowered)
{
	if (powered && !wasPowered)
	{
		world->playSoundEffect((double)x + 0.5, (double)y + 0.1, (double)z + 0.5, "random.click", 0.4f, 0.6f);
	}
	else if (!powered && wasPowered)
	{
		world->playSoundEffect((double)x + 0.5, (double)y + 0.1, (double)z + 0.5, "random.click", 0.4f, 0.5f);
	}
	else if (connected && !wasConnected)
	{
		world->playSoundEffect((double)x + 0.5, (double)y + 0.1, (double)z + 0.5, "random.click", 0.4f, 0.7f);
	}
	else if (!connected && wasConnected)
	{
		world->playSoundEffect((double)x + 0.5, (double)y + 0.1, (double)z + 0.5, "random.bowhit", 0.4f, 1.2f / (world->rand.nextFloat() * 0.2f + 0.9f));
	}
}

void BlockTripWireSource::notifyNeighborOfChange(World *world, int_t x, int_t y, int_t z, int_t direction)
{
	world->notifyBlocksOfNeighborChange(x, y, z, blockID);
	if (direction == 3) world->notifyBlocksOfNeighborChange(x - 1, y, z, blockID);
	else if (direction == 1) world->notifyBlocksOfNeighborChange(x + 1, y, z, blockID);
	else if (direction == 0) world->notifyBlocksOfNeighborChange(x, y, z - 1, blockID);
	else if (direction == 2) world->notifyBlocksOfNeighborChange(x, y, z + 1, blockID);
}

void BlockTripWireSource::updateTick(World *world, int_t x, int_t y, int_t z, Random &random)
{
	(void)random;
	func_72143_a(world, x, y, z, blockID, world->getBlockMetadata(x, y, z), true, -1, 0);
}

void BlockTripWireSource::setBlockBoundsBasedOnState(IBlockAccess *iblockaccess, int_t x, int_t y, int_t z)
{
	int_t dir = iblockaccess->getBlockMetadata(x, y, z) & 3;
	float f = 0.1875f;
	if (dir == 3) setBlockBounds(0.0f, 0.2f, 0.5f - f, f * 2.0f, 0.8f, 0.5f + f);
	else if (dir == 1) setBlockBounds(1.0f - f * 2.0f, 0.2f, 0.5f - f, 1.0f, 0.8f, 0.5f + f);
	else if (dir == 0) setBlockBounds(0.5f - f, 0.2f, 0.0f, 0.5f + f, 0.8f, f * 2.0f);
	else if (dir == 2) setBlockBounds(0.5f - f, 0.2f, 1.0f - f * 2.0f, 0.5f + f, 0.8f, 1.0f);
}

void BlockTripWireSource::onBlockRemoval(World *world, int_t x, int_t y, int_t z)
{
	int_t meta = world->getBlockMetadata(x, y, z);
	bool connected = (meta & 4) == 4;
	bool powered = (meta & 8) == 8;
	if (connected || powered)
	{
		func_72143_a(world, x, y, z, 0, meta, false, -1, 0);
	}
	if (powered)
	{
		world->notifyBlocksOfNeighborChange(x, y, z, blockID);
		int_t dir = meta & 3;
		if (dir == 3) world->notifyBlocksOfNeighborChange(x - 1, y, z, blockID);
		else if (dir == 1) world->notifyBlocksOfNeighborChange(x + 1, y, z, blockID);
		else if (dir == 0) world->notifyBlocksOfNeighborChange(x, y, z - 1, blockID);
		else if (dir == 2) world->notifyBlocksOfNeighborChange(x, y, z + 1, blockID);
	}
	Block::onBlockRemoval(world, x, y, z);
}

int_t BlockTripWireSource::isProvidingWeakPower(IBlockAccess *iblockaccess, int_t x, int_t y, int_t z, int_t side)
{
	(void)side;
	return (iblockaccess->getBlockMetadata(x, y, z) & 8) == 8 ? 15 : 0;
}

int_t BlockTripWireSource::isProvidingStrongPower(IBlockAccess *iblockaccess, int_t x, int_t y, int_t z, int_t side)
{
	int_t meta = iblockaccess->getBlockMetadata(x, y, z);
	if ((meta & 8) != 8) return 0;
	int_t dir = meta & 3;
	if (dir == 2 && side == 2) return 15;
	if (dir == 0 && side == 3) return 15;
	if (dir == 1 && side == 4) return 15;
	if (dir == 3 && side == 5) return 15;
	return 0;
}

bool BlockTripWireSource::isPoweringTo(IBlockAccess *iblockaccess, int_t x, int_t y, int_t z, int_t side)
{
	return isProvidingWeakPower(iblockaccess, x, y, z, side) > 0;
}

bool BlockTripWireSource::isIndirectlyPoweringTo(World *world, int_t x, int_t y, int_t z, int_t side)
{
	return isProvidingStrongPower(world, x, y, z, side) > 0;
}

bool BlockTripWireSource::canProvidePower()
{
	return true;
}
