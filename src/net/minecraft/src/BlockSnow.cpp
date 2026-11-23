#include "BlockSnow.h"
#include "Material.h"
#include "World.h"
#include "Block.h"
#include "BlockLeaves.h"
#include "AxisAlignedBB.h"
#include "IBlockAccess.h"
#include "Item.h"
#include "EntityItem.h"
#include "ItemStack.h"
#include "StatList.h"
#include "EntityPlayer.h"
#include "EnumSkyBlock.h"

BlockSnow::BlockSnow(int_t i, int_t j) : Block(i, j, Material::snow)
{
	setBlockBoundsForSnowDepth(0);
	setTickOnLoad(true);
}

AxisAlignedBB *BlockSnow::getCollisionBoundingBoxFromPool(World *world, int_t i, int_t j, int_t k)
{
	int_t l = world->getBlockMetadata(i, j, k) & 7;
	float f = 0.125f;
	return AxisAlignedBB::getBoundingBoxFromPool((double)i + minX, (double)j + minY, (double)k + minZ,
	                                             (double)i + maxX, (double)((float)j + (float)l * f), (double)k + maxZ);
}

bool BlockSnow::isOpaqueCube()        { return false; }
bool BlockSnow::renderAsNormalBlock() { return false; }

void BlockSnow::setBlockBoundsForItemRender()
{
	setBlockBoundsForSnowDepth(0);
}

void BlockSnow::setBlockBoundsBasedOnState(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k)
{
	setBlockBoundsForSnowDepth(iblockaccess->getBlockMetadata(i, j, k));
}

void BlockSnow::setBlockBoundsForSnowDepth(int_t meta)
{
	int_t l = meta & 7;
	float f = (float)(2 * (1 + l)) / 16.0f;
	setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, f, 1.0f);
}

bool BlockSnow::canPlaceBlockAt(World *world, int_t i, int_t j, int_t k)
{
	int_t l = world->getBlockId(i, j - 1, k);
	Block *below = (l > 0 && l < BLOCK_REGISTRY_SIZE) ? Block::blocksList[l] : nullptr;
	if (below == nullptr) return false;
	if (below == this && (world->getBlockMetadata(i, j - 1, k) & 7) == 7) return true;
	if (l != Block::leaves->blockID && !below->isOpaqueCube()) return false;
	return world->getBlockMaterial(i, j - 1, k)->blocksMovement();
}

void BlockSnow::onNeighborBlockChange(World *world, int_t i, int_t j, int_t k, int_t l)
{
	getGlowing(world, i, j, k);
}

bool BlockSnow::getGlowing(World *world, int_t i, int_t j, int_t k)
{
	if (!canPlaceBlockAt(world, i, j, k))
	{
		dropBlockAsItem(world, i, j, k, world->getBlockMetadata(i, j, k));
		world->setBlockWithNotify(i, j, k, 0);
		return false;
	}
	return true;
}

void BlockSnow::harvestBlock(World *world, EntityPlayer *entityplayer, int_t i, int_t j, int_t k, int_t l)
{
	int_t i1 = Item::snowball->shiftedIndex;
	int_t count = (l & 7) + 1;
	dropBlockAsItem_do(world, i, j, k, new ItemStack(i1, count, 0));
	world->setBlockWithNotify(i, j, k, 0);
	entityplayer->addStat(StatList::mineBlockStatArray[blockID], 1);
}

int_t BlockSnow::idDropped(int_t i, Random &random)
{
	return Item::snowball->shiftedIndex;
}

int_t BlockSnow::quantityDropped(Random &random) { return 0; }

void BlockSnow::updateTick(World *world, int_t i, int_t j, int_t k, Random &random)
{
	if (world->getSavedLightValue(EnumSkyBlock::Block, i, j, k) > 11)
	{
		dropBlockAsItem(world, i, j, k, world->getBlockMetadata(i, j, k));
		world->setBlockWithNotify(i, j, k, 0);
	}
}

bool BlockSnow::shouldSideBeRendered(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k, int_t l)
{
#ifdef PS2_PLATFORM
	// Snow layers always sit on a supporting full-height surface, so their
	// underside can never contribute visible pixels. The vanilla rule only
	// rejects it when that support reports itself opaque (leaves do not).
	if (l == 0)
		return false;

	// The top of a snow layer is always exposed inside its own block cell.
	if (l == 1)
		return true;

	// A snow layer is non-opaque, so Block::shouldSideBeRendered() treats an
	// adjacent snow layer as transparent and emits both sides of every internal
	// seam. Large snowy biomes therefore submit five quads per layer even on a
	// perfectly flat field. Cull a horizontal side when the neighbouring snow
	// reaches at least as high as this layer; keep the side when the neighbour is
	// lower so stacked/uneven snow still shows its exposed step.
	if (iblockaccess->getBlockId(i, j, k) == blockID)
	{
		int_t selfX = i;
		int_t selfZ = k;
		switch (l)
		{
		case 2: ++selfZ; break;
		case 3: --selfZ; break;
		case 4: ++selfX; break;
		case 5: --selfX; break;
		default: break;
		}

		const int_t neighbourHeight = iblockaccess->getBlockMetadata(i, j, k) & 7;
		const int_t selfHeight = iblockaccess->getBlockMetadata(selfX, j, selfZ) & 7;
		if (neighbourHeight >= selfHeight)
			return false;
	}
#else
	if (l == 1) return true;
#endif

	return Block::shouldSideBeRendered(iblockaccess, i, j, k, l);
}
