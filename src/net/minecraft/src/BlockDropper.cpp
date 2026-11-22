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

	const int_t slot = tile->getRandomSlotFromInventory();
	if (slot < 0)
	{
		world->playAuxSFX(1001, i, j, k, 0);
		return;
	}

	ItemStack *slotStack = tile->getStackInSlot(slot);
	if (slotStack == nullptr || slotStack->stackSize <= 0)
	{
		world->playAuxSFX(1001, i, j, k, 0);
		return;
	}

	const int_t meta = world->getBlockMetadata(i, j, k) & 7;
	const int_t safeMeta = (meta >= 0 && meta < 6) ? meta : 3;
	const int_t dx = FacingUtil::offsetsXForSide[safeMeta];
	const int_t dy = FacingUtil::offsetsYForSide[safeMeta];
	const int_t dz = FacingUtil::offsetsZForSide[safeMeta];
	const int_t targetX = i + dx;
	const int_t targetY = j + dy;
	const int_t targetZ = k + dz;

	IInventory *targetInv = TileEntityHopper::getInventoryAtLocation(world, (double)targetX, (double)targetY, (double)targetZ);
	if (targetInv != nullptr)
	{
		ItemStack *one = slotStack->copy();
		one->stackSize = 1;
		ItemStack *leftover = TileEntityHopper::insertStack(targetInv, one, FacingUtil::oppositeSide[safeMeta]);
		if (leftover == nullptr)
		{
			ItemStack *consumed = tile->decrStackSize(slot, 1);
			delete consumed;
			tile->onInventoryChanged();
		}
		else
		{
			delete leftover;
		}
	}
	else
	{
		ItemStack *popped = tile->decrStackSize(slot, 1);
		if (popped == nullptr)
			return;
		double x = (double)i + (double)dx * 0.7 + 0.5;
		double y = (double)j + (double)dy * 0.7 + 0.5;
		double z = (double)k + (double)dz * 0.7 + 0.5;
		if (dy == 0)
			y -= 0.15625;
		else
			y -= 0.125;

		EntityItem *entityItem = new EntityItem(world, x, y, z, popped);
		const double speed = random.nextDouble() * 0.1 + 0.2;
		entityItem->motionX = (double)dx * speed;
		entityItem->motionY = (dy == 0) ? 0.2 : ((double)dy * speed);
		entityItem->motionZ = (double)dz * speed;
		entityItem->motionX += random.nextGaussian() * 0.0075 * 6.0;
		entityItem->motionY += random.nextGaussian() * 0.0075 * 6.0;
		entityItem->motionZ += random.nextGaussian() * 0.0075 * 6.0;

		if (!world->entityJoinedWorld(entityItem))
		{
			delete entityItem;
		}
		world->playAuxSFX(1000, i, j, k, 0);
		const int_t smokeParam = (safeMeta == 0) ? 9 : ((safeMeta == 1) ? 10 : (dx + 1 + (dz + 1) * 3));
		world->playAuxSFX(2000, i, j, k, smokeParam);
	}
}
