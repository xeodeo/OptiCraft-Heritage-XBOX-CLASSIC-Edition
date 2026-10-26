#include "BlockDropper.h"
#include "Block.h"
#include "BlockPistonBase.h"
#include "EntityItem.h"
#include "EntityLiving.h"
#include "Facing.h"
#include "IBlockAccess.h"
#include "ItemStack.h"
#include "Texture152Atlas.h"
#include "TileEntityDropper.h"
#include "TileEntityHopper.h"
#include "World.h"

BlockDropper::BlockDropper(int_t id)
	: BlockDispenser(id)
{
	blockIndexInTexture = 45;
}

int_t BlockDropper::idDropped(int_t i, Random &random)
{
	(void)i;
	(void)random;
	return Block::dropper != nullptr ? Block::dropper->blockID : blockID;
}

int_t BlockDropper::getBlockTexture(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k, int_t l)
{
	int_t meta = iblockaccess->getBlockMetadata(i, j, k) & 7;
	if (l == meta)
	{
		return (meta == 0 || meta == 1)
			? TEX152_BLOCK_ICON("dropper_front_vertical.png")
			: TEX152_BLOCK_ICON("dropper_front.png");
	}
	if (meta == 0 || meta == 1)
	{
		return 62;
	}
	return (l == 0 || l == 1) ? 62 : 45;
}

int_t BlockDropper::getBlockTextureFromSide(int_t i)
{
	if (i == 3)
	{
		return TEX152_BLOCK_ICON("dropper_front.png");
	}
	if (i == 0 || i == 1)
	{
		return 62;
	}
	return 45;
}

TileEntity *BlockDropper::getBlockEntity()
{
	return new TileEntityDropper();
}

void BlockDropper::onBlockPlacedBy(World *world, int_t i, int_t j, int_t k, EntityLiving *entityliving)
{
	int_t orientation = BlockPistonBase::getOrientationFromEntity(world, i, j, k, entityliving);
	world->setBlockMetadataWithNotify152(i, j, k, orientation, 2);
}

void BlockDropper::onNeighborBlockChange(World *world, int_t i, int_t j, int_t k, int_t l)
{
	(void)l;
	bool powered = world->isBlockIndirectlyGettingPowered(i, j, k) ||
	               world->isBlockIndirectlyGettingPowered(i, j + 1, k);
	int_t meta = world->getBlockMetadata(i, j, k);
	bool wasPowered = (meta & 8) != 0;
	if (powered && !wasPowered)
	{
		world->scheduleBlockUpdate(i, j, k, blockID, tickRate());
		world->setBlockMetadataWithNotify152(i, j, k, meta | 8, 4);
	}
	else if (!powered && wasPowered)
	{
		world->setBlockMetadataWithNotify152(i, j, k, meta & ~8, 4);
	}
}

void BlockDropper::updateTick(World *world, int_t i, int_t j, int_t k, Random &random)
{
	if (!world->multiplayerWorld)
	{
		dispenseItem(world, i, j, k, random);
	}
}

void BlockDropper::dispenseItem(World *world, int_t i, int_t j, int_t k, Random &random)
{
	TileEntityDispenser *tile = dynamic_cast<TileEntityDispenser *>(world->getBlockTileEntity(i, j, k));
	if (tile == nullptr)
	{
		return;
	}

	ItemStack *stack = tile->getRandomStackFromInventory();
	if (stack == nullptr)
	{
		world->isTrapdoorOpen(1001, i, j, k, 0);
		return;
	}

	int_t meta = world->getBlockMetadata(i, j, k) & 7;
	int_t targetX = i + FacingUtil::offsetsXForSide[meta];
	int_t targetY = j + FacingUtil::offsetsYForSide[meta];
	int_t targetZ = k + FacingUtil::offsetsZForSide[meta];

	IInventory *targetInv = TileEntityHopper::getInventoryAtLocation(world, (double)targetX, (double)targetY, (double)targetZ);
	if (targetInv != nullptr)
	{
		ItemStack *leftover = TileEntityHopper::insertStack(targetInv, stack, FacingUtil::oppositeSide[meta]);
		if (leftover != nullptr)
		{
			TileEntityHopper::insertStack(tile, leftover, -1);
		}
	}
	else
	{
		double x = (double)i + (double)FacingUtil::offsetsXForSide[meta] * 0.6 + 0.5;
		double y = (double)j + (double)FacingUtil::offsetsYForSide[meta] * 0.6 + 0.5;
		double z = (double)k + (double)FacingUtil::offsetsZForSide[meta] * 0.6 + 0.5;
		if (FacingUtil::offsetsYForSide[meta] == 0)
		{
			y -= 0.1;
		}

		EntityItem *entityItem = new EntityItem(world, x, y, z, stack);
		double speed = random.nextDouble() * 0.1 + 0.2;
		entityItem->motionX = (double)FacingUtil::offsetsXForSide[meta] * speed;
		entityItem->motionY = 0.2;
		entityItem->motionZ = (double)FacingUtil::offsetsZForSide[meta] * speed;
		if (FacingUtil::offsetsYForSide[meta] != 0)
		{
			entityItem->motionY = (double)FacingUtil::offsetsYForSide[meta] * speed;
		}

		entityItem->motionX += random.nextGaussian() * 0.0075 * 6.0;
		entityItem->motionY += random.nextGaussian() * 0.0075 * 6.0;
		entityItem->motionZ += random.nextGaussian() * 0.0075 * 6.0;

		if (!world->entityJoinedWorld(entityItem))
		{
			delete entityItem;
		}
		world->isTrapdoorOpen(1000, i, j, k, 0);
	}
}
