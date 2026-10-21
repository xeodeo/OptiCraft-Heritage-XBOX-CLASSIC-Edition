#include "BlockCocoa.h"
#include "Texture152Atlas.h"

#include "AxisAlignedBB.h"
#include "Block.h"
#include "EntityLiving.h"
#include "IBlockAccess.h"
#include "Item.h"
#include "ItemStack.h"
#include "Material.h"
#include "MathHelper.h"
#include "World.h"

BlockCocoa::BlockCocoa(int_t id)
	: BlockDirectional(id, TEX152_BLOCK_ICON("cocoa_2.png"), Material::plants) // 1.5.2 icon: cocoa_2.png
{
	setTickRandomly(true);
}

int_t BlockCocoa::getCocoaIconForStage(int_t stage)
{
	if (stage <= 0) return TEX152_BLOCK_ICON("cocoa_0.png"); // 1.5.2 icon: cocoa_0.png
	if (stage == 1) return TEX152_BLOCK_ICON("cocoa_1.png"); // 1.5.2 icon: cocoa_1.png
	return TEX152_BLOCK_ICON("cocoa_2.png"); // 1.5.2 icon: cocoa_2.png
}

int_t BlockCocoa::getBlockTextureFromSideAndMetadata(int_t side, int_t metadata)
{
	(void)side;
	return getCocoaIconForStage(func_72219_c(metadata));
}

int_t BlockCocoa::getBlockTextureFromSide(int_t side)
{
	(void)side;
	return getCocoaIconForStage(2);
}

void BlockCocoa::updateTick(World *world, int_t i, int_t j, int_t k, Random &random)
{
	if (!canBlockStay(world, i, j, k))
	{
		dropBlockAsItem(world, i, j, k, world->getBlockMetadata(i, j, k), 0);
		world->setBlockWithNotify(i, j, k, 0);
	}
	else if (random.nextInt(5) == 0)
	{
		const int_t meta = world->getBlockMetadata(i, j, k);
		int_t stage = func_72219_c(meta);
		if (stage < 2)
		{
			++stage;
			world->setBlockMetadataWithNotify(i, j, k, (stage << 2) | getDirection(meta));
		}
	}
}

bool BlockCocoa::canBlockStay(World *world, int_t i, int_t j, int_t k)
{
	const int_t dir = getDirection(world->getBlockMetadata(i, j, k));
	static const int_t dirOffsetX[4] = {0, -1, 0, 1};
	static const int_t dirOffsetZ[4] = {1, 0, -1, 0};
	i += dirOffsetX[dir];
	k += dirOffsetZ[dir];
	const int_t id = world->getBlockId(i, j, k);
	return (Block::wood != nullptr && id == Block::wood->blockID) && ((world->getBlockMetadata(i, j, k) & 3) == 3);
}

void BlockCocoa::onNeighborBlockChange(World *world, int_t i, int_t j, int_t k, int_t l)
{
	(void)l;
	if (!canBlockStay(world, i, j, k))
	{
		dropBlockAsItem(world, i, j, k, world->getBlockMetadata(i, j, k), 0);
		world->setBlockWithNotify(i, j, k, 0);
	}
}

void BlockCocoa::setBlockBoundsBasedOnState(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k)
{
	const int_t meta = iblockaccess->getBlockMetadata(i, j, k);
	const int_t dir = getDirection(meta);
	const int_t stage = func_72219_c(meta);
	const int_t w = 4 + stage * 2;
	const int_t h = 5 + stage * 2;
	const float halfW = (float)w / 2.0f;
	switch (dir)
	{
	case 0:
		setBlockBounds((8.0f - halfW) / 16.0f, (12.0f - (float)h) / 16.0f, (15.0f - (float)w) / 16.0f, (8.0f + halfW) / 16.0f, 0.75f, 0.9375f);
		break;
	case 1:
		setBlockBounds(0.0625f, (12.0f - (float)h) / 16.0f, (8.0f - halfW) / 16.0f, (1.0f + (float)w) / 16.0f, 0.75f, (8.0f + halfW) / 16.0f);
		break;
	case 2:
		setBlockBounds((8.0f - halfW) / 16.0f, (12.0f - (float)h) / 16.0f, 0.0625f, (8.0f + halfW) / 16.0f, 0.75f, (1.0f + (float)w) / 16.0f);
		break;
	case 3:
		setBlockBounds((15.0f - (float)w) / 16.0f, (12.0f - (float)h) / 16.0f, (8.0f - halfW) / 16.0f, 0.9375f, 0.75f, (8.0f + halfW) / 16.0f);
		break;
	}
}

AxisAlignedBB *BlockCocoa::getCollisionBoundingBoxFromPool(World *world, int_t i, int_t j, int_t k)
{
	setBlockBoundsBasedOnState(world, i, j, k);
	return BlockDirectional::getCollisionBoundingBoxFromPool(world, i, j, k);
}

AxisAlignedBB *BlockCocoa::getSelectedBoundingBoxFromPool(World *world, int_t i, int_t j, int_t k)
{
	setBlockBoundsBasedOnState(world, i, j, k);
	return BlockDirectional::getSelectedBoundingBoxFromPool(world, i, j, k);
}

void BlockCocoa::onBlockPlacedBy(World *world, int_t i, int_t j, int_t k, EntityLiving *entityliving)
{
	const int_t dir = ((MathHelper::floor_double((double)(entityliving->rotationYaw * 4.0f / 360.0f) + 0.5) & 3) + 0) % 4;
	world->setBlockMetadataWithNotify(i, j, k, dir);
}

int_t BlockCocoa::onBlockPlaced(World *world, int_t i, int_t j, int_t k, int_t side, float hitX, float hitY, float hitZ, int_t metadata)
{
	(void)world;
	(void)i;
	(void)j;
	(void)k;
	(void)hitX;
	(void)hitY;
	(void)hitZ;
	(void)metadata;
	if (side == 1 || side == 0)
	{
		side = 2;
	}
	static const int_t facingToDir[6] = {-1, -1, 2, 0, 1, 3};
	static const int_t rotateOpp[4] = {2, 3, 0, 1};
	return rotateOpp[facingToDir[side]];
}

void BlockCocoa::onBlockPlaced(World *world, int_t i, int_t j, int_t k, int_t side)
{
	const int_t dir = onBlockPlaced(world, i, j, k, side, 0.0f, 0.0f, 0.0f, 0);
	world->setBlockMetadataWithNotify(i, j, k, dir);
}

void BlockCocoa::dropBlockAsItemWithChance(World *world, int_t i, int_t j, int_t k, int_t metadata, float chance, int_t fortune)
{
	(void)chance;
	(void)fortune;
	const int_t stage = func_72219_c(metadata);
	const int_t count = (stage >= 2) ? 3 : 1;
	for (int_t n = 0; n < count; ++n)
	{
		dropBlockAsItem_do(world, i, j, k, new ItemStack(Item::dyePowder, 1, 3));
	}
}

int_t BlockCocoa::idPicked(World *world, int_t i, int_t j, int_t k)
{
	(void)world; (void)i; (void)j; (void)k;
	return Item::dyePowder != nullptr ? Item::dyePowder->shiftedIndex : 351;
}

int_t BlockCocoa::getDamageValue(World *world, int_t i, int_t j, int_t k)
{
	(void)world; (void)i; (void)j; (void)k;
	return 3;
}
