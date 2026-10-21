#include "BlockTripWire.h"
#include "AxisAlignedBB.h"
#include "BlockTripWireSource.h"
#include "Direction.h"
#include "EntityPlayer.h"
#include "IBlockAccess.h"
#include "Item.h"
#include "ItemStack.h"
#include "Material.h"
#include "Texture152Atlas.h"
#include "World.h"

BlockTripWire::BlockTripWire(int_t id)
	: Block(id, Material::circuits)
{
	blockIndexInTexture = TEX152_BLOCK_ICON("tripWire.png");
	setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 0.15625f, 1.0f);
	setTickOnLoad(true);
}

int_t BlockTripWire::tickRate()
{
	return 10;
}

AxisAlignedBB *BlockTripWire::getCollisionBoundingBoxFromPool(World *world, int_t x, int_t y, int_t z)
{
	(void)world; (void)x; (void)y; (void)z;
	return nullptr;
}

bool BlockTripWire::isOpaqueCube()
{
	return false;
}

bool BlockTripWire::renderAsNormalBlock()
{
	return false;
}

int_t BlockTripWire::getRenderBlockPass()
{
	return 1;
}

int_t BlockTripWire::getRenderType()
{
	return 30;
}

int_t BlockTripWire::idDropped(int_t meta, Random &random, int_t fortune)
{
	(void)meta; (void)random; (void)fortune;
	return Item::silk != nullptr ? Item::silk->shiftedIndex : 287;
}

void BlockTripWire::onNeighborBlockChange(World *world, int_t x, int_t y, int_t z, int_t neighborId)
{
	(void)neighborId;
	int_t meta = world->getBlockMetadata(x, y, z);
	bool suspended = (meta & 2) == 2;
	bool hasNoSupport = !world->isBlockNormalCube(x, y - 1, z);
	if (suspended != hasNoSupport)
	{
		dropBlockAsItem(world, x, y, z, meta);
		world->setBlockWithNotify(x, y, z, 0);
	}
}

void BlockTripWire::setBlockBoundsBasedOnState(IBlockAccess *iblockaccess, int_t x, int_t y, int_t z)
{
	int_t meta = iblockaccess->getBlockMetadata(x, y, z);
	bool connected = (meta & 4) == 4;
	bool suspended = (meta & 2) == 2;
	if (!suspended)
	{
		setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 0.09375f, 1.0f);
	}
	else if (!connected)
	{
		setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f);
	}
	else
	{
		setBlockBounds(0.0f, 0.0625f, 0.0f, 1.0f, 0.15625f, 1.0f);
	}
}

void BlockTripWire::onBlockAdded(World *world, int_t x, int_t y, int_t z)
{
	int_t meta = world->isBlockNormalCube(x, y - 1, z) ? 0 : 2;
	world->setBlockMetadataWithNotify(x, y, z, meta);
	notifyTripWireSources(world, x, y, z, meta);
}

void BlockTripWire::onBlockRemoval(World *world, int_t x, int_t y, int_t z)
{
	int_t meta = world->getBlockMetadata(x, y, z);
	notifyTripWireSources(world, x, y, z, meta | 1);
	Block::onBlockRemoval(world, x, y, z);
}

void BlockTripWire::harvestBlock(World *world, EntityPlayer *player, int_t x, int_t y, int_t z, int_t meta)
{
	if (!world->multiplayerWorld && player != nullptr && player->getCurrentEquippedItem() != nullptr &&
	    Item::shears != nullptr && player->getCurrentEquippedItem()->itemID == Item::shears->shiftedIndex)
	{
		world->setBlockMetadata(x, y, z, meta | 8);
	}
	Block::harvestBlock(world, player, x, y, z, meta);
}

void BlockTripWire::notifyTripWireSources(World *world, int_t x, int_t y, int_t z, int_t meta)
{
	for (int_t dir = 0; dir < 2; ++dir)
	{
		for (int_t dist = 1; dist < 42; ++dist)
		{
			int_t curX = x + Direction::offsetX[dir] * dist;
			int_t curZ = z + Direction::offsetZ[dir] * dist;
			int_t bId = world->getBlockId(curX, y, curZ);
			if (Block::tripWireSource != nullptr && bId == Block::tripWireSource->blockID)
			{
				int_t srcDir = world->getBlockMetadata(curX, y, curZ) & 3;
				if (srcDir == Direction::rotateOpposite[dir])
				{
					static_cast<BlockTripWireSource*>(Block::tripWireSource)->func_72143_a(
						world, curX, y, curZ, bId, world->getBlockMetadata(curX, y, curZ), true, dist, meta);
				}
				break;
			}
			if (Block::tripWire == nullptr || bId != Block::tripWire->blockID)
			{
				break;
			}
		}
	}
}

void BlockTripWire::onEntityCollidedWithBlock(World *world, int_t x, int_t y, int_t z, Entity *entity)
{
	(void)entity;
	if (!world->multiplayerWorld)
	{
		if ((world->getBlockMetadata(x, y, z) & 1) != 1)
		{
			updateTripWireState(world, x, y, z);
		}
	}
}

void BlockTripWire::updateTick(World *world, int_t x, int_t y, int_t z, Random &random)
{
	(void)random;
	if (!world->multiplayerWorld)
	{
		if ((world->getBlockMetadata(x, y, z) & 1) == 1)
		{
			updateTripWireState(world, x, y, z);
		}
	}
}

void BlockTripWire::updateTripWireState(World *world, int_t x, int_t y, int_t z)
{
	int_t meta = world->getBlockMetadata(x, y, z);
	bool tripped = (meta & 1) == 1;
	bool hasEntities = false;
	AxisAlignedBB *aabb = AxisAlignedBB::getBoundingBoxFromPool(
		(double)x + minX, (double)y + minY, (double)z + minZ,
		(double)x + maxX, (double)y + maxY, (double)z + maxZ);
	auto list = world->getEntitiesWithinAABBExcludingEntity(nullptr, aabb);
	if (!list.empty())
	{
		hasEntities = true;
	}
	if (hasEntities && !tripped)
	{
		meta |= 1;
	}
	if (!hasEntities && tripped)
	{
		meta &= ~1;
	}
	if (hasEntities != tripped)
	{
		world->setBlockMetadataWithNotify(x, y, z, meta);
		notifyTripWireSources(world, x, y, z, meta);
	}
	if (hasEntities)
	{
		world->scheduleBlockUpdate(x, y, z, blockID, tickRate());
	}
}

bool BlockTripWire::isConnected(IBlockAccess *access, int_t x, int_t y, int_t z, int_t meta, int_t direction)
{
	int_t curX = x + Direction::offsetX[direction];
	int_t curZ = z + Direction::offsetZ[direction];
	int_t bId = access->getBlockId(curX, y, curZ);
	bool suspended = (meta & 2) == 2;
	if (Block::tripWireSource != nullptr && bId == Block::tripWireSource->blockID)
	{
		int_t srcMeta = access->getBlockMetadata(curX, y, curZ);
		int_t srcDir = srcMeta & 3;
		return srcDir == Direction::rotateOpposite[direction];
	}
	else if (Block::tripWire != nullptr && bId == Block::tripWire->blockID)
	{
		int_t otherMeta = access->getBlockMetadata(curX, y, curZ);
		bool otherSuspended = (otherMeta & 2) == 2;
		return suspended == otherSuspended;
	}
	return false;
}
