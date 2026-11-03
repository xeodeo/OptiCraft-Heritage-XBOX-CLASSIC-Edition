#include "BlockWall.h"

#include "AxisAlignedBB.h"
#include "IBlockAccess.h"
#include "ItemStack.h"
#include "Material.h"
#include "World.h"

const char *const BlockWall::types[2] = {"normal", "mossy"};

BlockWall::BlockWall(int_t id, Block *modelBlock)
	: Block(id, modelBlock->blockMaterial)
{
	setHardness(modelBlock->blockHardness);
	setResistance(modelBlock->blockResistance / 3.0f);
	setStepSound(modelBlock->stepSound);
}

int_t BlockWall::getBlockTextureFromSideAndMetadata(int_t side, int_t metadata)
{
	if (metadata == 1 && Block::cobblestoneMossy != nullptr)
	{
		return Block::cobblestoneMossy->getBlockTextureFromSide(side);
	}
	if (Block::cobblestone != nullptr)
	{
		return Block::cobblestone->getBlockTextureFromSide(side);
	}
	return blockIndexInTexture;
}

bool BlockWall::canConnectWallTo(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k)
{
	const int_t id = iblockaccess->getBlockId(i, j, k);
	if (id != blockID && (Block::fenceGate == nullptr || id != Block::fenceGate->blockID))
	{
		Block *block = Block::blocksList[id];
		return (block != nullptr && block->blockMaterial != nullptr && block->blockMaterial->isOpaque() && block->renderAsNormalBlock()) ? (block->blockMaterial != Material::pumpkin) : false;
	}
	return true;
}

void BlockWall::setBlockBoundsBasedOnState(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k)
{
	const bool north = canConnectWallTo(iblockaccess, i, j, k - 1);
	const bool south = canConnectWallTo(iblockaccess, i, j, k + 1);
	const bool west = canConnectWallTo(iblockaccess, i - 1, j, k);
	const bool east = canConnectWallTo(iblockaccess, i + 1, j, k);
	float minX = 0.25f;
	float maxX = 0.75f;
	float minZ = 0.25f;
	float maxZ = 0.75f;
	float maxY = 1.0f;
	if (north) minZ = 0.0f;
	if (south) maxZ = 1.0f;
	if (west) minX = 0.0f;
	if (east) maxX = 1.0f;
	if (north && south && !west && !east)
	{
		maxY = 0.8125f;
		minX = 0.3125f;
		maxX = 0.6875f;
	}
	else if (!north && !south && west && east)
	{
		maxY = 0.8125f;
		minZ = 0.3125f;
		maxZ = 0.6875f;
	}
	setBlockBounds(minX, 0.0f, minZ, maxX, maxY, maxZ);
}

AxisAlignedBB *BlockWall::getCollisionBoundingBoxFromPool(World *world, int_t i, int_t j, int_t k)
{
	setBlockBoundsBasedOnState(world, i, j, k);
	maxY = 1.5;
	return Block::getCollisionBoundingBoxFromPool(world, i, j, k);
}

bool BlockWall::shouldSideBeRendered(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k, int_t side)
{
	return side == 0 ? Block::shouldSideBeRendered(iblockaccess, i, j, k, side) : true;
}

void BlockWall::getSubBlocks(int_t id, CreativeTabs *tab, std::vector<ItemStack *> &list)
{
	(void)tab;
	list.push_back(new ItemStack(id, 1, 0));
	list.push_back(new ItemStack(id, 1, 1));
}
