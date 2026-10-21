#include "BlockHalfSlab.h"

#include "AxisAlignedBB.h"
#include "IBlockAccess.h"
#include "Material.h"
#include "World.h"

BlockHalfSlab::BlockHalfSlab(int_t id, bool isDouble, Material *material)
	: Block(id, material)
	, isDoubleSlab(isDouble)
{
	if (isDouble)
	{
		opaqueCubeLookup[id] = true;
	}
	else
	{
		setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f);
	}
	setLightOpacity(255);
}

void BlockHalfSlab::setBlockBoundsBasedOnState(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k)
{
	if (isDoubleSlab)
	{
		setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
		return;
	}

	if ((iblockaccess->getBlockMetadata(i, j, k) & 8) != 0)
	{
		setBlockBounds(0.0f, 0.5f, 0.0f, 1.0f, 1.0f, 1.0f);
	}
	else
	{
		setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f);
	}
}

void BlockHalfSlab::setBlockBoundsForItemRender()
{
	if (isDoubleSlab)
	{
		setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
	}
	else
	{
		setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f);
	}
}

void BlockHalfSlab::getCollidingBoundingBoxes(World *world, int_t i, int_t j, int_t k, AxisAlignedBB *axisalignedbb, std::vector<AxisAlignedBB *> &arraylist)
{
	setBlockBoundsBasedOnState(world, i, j, k);
	Block::getCollidingBoundingBoxes(world, i, j, k, axisalignedbb, arraylist);
}

bool BlockHalfSlab::isOpaqueCube()
{
	return isDoubleSlab;
}

void BlockHalfSlab::onBlockPlaced(World *world, int_t i, int_t j, int_t k, int_t side)
{
	if (side == 0 && !isDoubleSlab)
	{
		const int_t metadata = world->getBlockMetadata(i, j, k) & 7;
		world->setBlockMetadataWithNotify(i, j, k, metadata | 8);
	}
}

int_t BlockHalfSlab::onBlockPlaced(World *world, int_t i, int_t j, int_t k, int_t side, float hitX, float hitY, float hitZ, int_t metadata)
{
	(void)hitX;
	(void)hitZ;
	if (isDoubleSlab)
	{
		return metadata;
	}
	return (side != 0 && (side == 1 || !((double)hitY > 0.5))) ? metadata : (metadata | 8);
}

int_t BlockHalfSlab::quantityDropped(Random &random)
{
	(void)random;
	return isDoubleSlab ? 2 : 1;
}

int_t BlockHalfSlab::damageDropped(int_t i)
{
	return i & 7;
}

bool BlockHalfSlab::renderAsNormalBlock()
{
	return isDoubleSlab;
}

bool BlockHalfSlab::shouldSideBeRendered(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k, int_t side)
{
	if (isDoubleSlab)
	{
		return Block::shouldSideBeRendered(iblockaccess, i, j, k, side);
	}

	if (side != 1 && side != 0 && !Block::shouldSideBeRendered(iblockaccess, i, j, k, side))
	{
		return false;
	}

	static const int_t opposite[6] = {1, 0, 3, 2, 5, 4};
	static const int_t offsetX[6] = {0, 0, 0, 0, -1, 1};
	static const int_t offsetY[6] = {-1, 1, 0, 0, 0, 0};
	static const int_t offsetZ[6] = {0, 0, -1, 1, 0, 0};
	const int_t adjacentSide = opposite[side];
	const int_t adjacentX = i + offsetX[adjacentSide];
	const int_t adjacentY = j + offsetY[adjacentSide];
	const int_t adjacentZ = k + offsetZ[adjacentSide];
	const bool adjacentTopHalf = (iblockaccess->getBlockMetadata(adjacentX, adjacentY, adjacentZ) & 8) != 0;

	if (!adjacentTopHalf)
	{
		if (side == 1) return true;
		if (side == 0 && Block::shouldSideBeRendered(iblockaccess, i, j, k, side)) return true;
		return !isBlockSingleSlab(iblockaccess->getBlockId(i, j, k)) || (iblockaccess->getBlockMetadata(i, j, k) & 8) != 0;
	}

	if (side == 0) return true;
	if (side == 1 && Block::shouldSideBeRendered(iblockaccess, i, j, k, side)) return true;
	return !isBlockSingleSlab(iblockaccess->getBlockId(i, j, k)) || (iblockaccess->getBlockMetadata(i, j, k) & 8) == 0;
}

bool BlockHalfSlab::isBlockSingleSlab(int_t id)
{
	return (Block::stairSingle != nullptr && id == Block::stairSingle->blockID) ||
	       (Block::woodSingleSlab != nullptr && id == Block::woodSingleSlab->blockID);
}
