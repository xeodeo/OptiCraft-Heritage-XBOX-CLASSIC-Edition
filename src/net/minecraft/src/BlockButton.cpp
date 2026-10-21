#include "BlockButton.h"
#include "Material.h"
#include "World.h"
#include "IBlockAccess.h"
#include "AxisAlignedBB.h"
#include "EntityPlayer.h"
#include "EntityArrow.h"
#include "Block.h"

BlockButton::BlockButton(int_t id, bool sensibleIn)
	: Block(id, Material::circuits),
	  sensible(sensibleIn)
{
	setTickOnLoad(true);
}

AxisAlignedBB *BlockButton::getCollisionBoundingBoxFromPool(World *world, int_t i, int_t j, int_t k)
{
	(void)world;
	(void)i;
	(void)j;
	(void)k;
	return nullptr;
}

int_t BlockButton::tickRate()
{
	return sensible ? 30 : 20;
}

bool BlockButton::isOpaqueCube()
{
	return false;
}

bool BlockButton::renderAsNormalBlock()
{
	return false;
}

bool BlockButton::canPlaceBlockOnSide(World *world, int_t i, int_t j, int_t k, int_t l)
{
	if (l == 2 && world->isBlockNormalCube(i, j, k + 1)) return true;
	if (l == 3 && world->isBlockNormalCube(i, j, k - 1)) return true;
	if (l == 4 && world->isBlockNormalCube(i + 1, j, k)) return true;
	return l == 5 && world->isBlockNormalCube(i - 1, j, k);
}

bool BlockButton::canPlaceBlockAt(World *world, int_t i, int_t j, int_t k)
{
	if (world->isBlockNormalCube(i - 1, j, k)) return true;
	if (world->isBlockNormalCube(i + 1, j, k)) return true;
	if (world->isBlockNormalCube(i, j, k - 1)) return true;
	return world->isBlockNormalCube(i, j, k + 1);
}

void BlockButton::onBlockPlaced(World *world, int_t i, int_t j, int_t k, int_t l)
{
	int_t meta = world->getBlockMetadata(i, j, k);
	int_t pressed = meta & 8;
	int_t side = meta & 7;

	if (l == 2 && world->isBlockNormalCube(i, j, k + 1)) side = 4;
	else if (l == 3 && world->isBlockNormalCube(i, j, k - 1)) side = 3;
	else if (l == 4 && world->isBlockNormalCube(i + 1, j, k)) side = 2;
	else if (l == 5 && world->isBlockNormalCube(i - 1, j, k)) side = 1;
	else side = getOrientation(world, i, j, k);

	world->setBlockMetadataWithNotify(i, j, k, side + pressed);
}

int_t BlockButton::getOrientation(World *world, int_t i, int_t j, int_t k)
{
	if (world->isBlockNormalCube(i - 1, j, k)) return 1;
	if (world->isBlockNormalCube(i + 1, j, k)) return 2;
	if (world->isBlockNormalCube(i, j, k - 1)) return 3;
	return !world->isBlockNormalCube(i, j, k + 1) ? 1 : 4;
}

void BlockButton::onNeighborBlockChange(World *world, int_t i, int_t j, int_t k, int_t l)
{
	(void)l;
	if (canPlaceButtonAt(world, i, j, k))
	{
		int_t side = world->getBlockMetadata(i, j, k) & 7;
		bool drop = false;
		if (!world->isBlockNormalCube(i - 1, j, k) && side == 1) drop = true;
		if (!world->isBlockNormalCube(i + 1, j, k) && side == 2) drop = true;
		if (!world->isBlockNormalCube(i, j, k - 1) && side == 3) drop = true;
		if (!world->isBlockNormalCube(i, j, k + 1) && side == 4) drop = true;
		if (drop)
		{
			dropBlockAsItem(world, i, j, k, world->getBlockMetadata(i, j, k));
			world->setBlockWithNotify(i, j, k, 0);
		}
	}
}

bool BlockButton::canPlaceButtonAt(World *world, int_t i, int_t j, int_t k)
{
	if (!canPlaceBlockAt(world, i, j, k))
	{
		dropBlockAsItem(world, i, j, k, world->getBlockMetadata(i, j, k));
		world->setBlockWithNotify(i, j, k, 0);
		return false;
	}
	return true;
}

void BlockButton::setButtonBounds(int_t meta)
{
	int_t side = meta & 7;
	bool pressed = (meta & 8) > 0;
	float yMin = 0.375f;
	float yMax = 0.625f;
	float halfWidth = 0.1875f;
	float depth = pressed ? 0.0625f : 0.125f;

	if (side == 1) setBlockBounds(0.0f, yMin, 0.5f - halfWidth, depth, yMax, 0.5f + halfWidth);
	else if (side == 2) setBlockBounds(1.0f - depth, yMin, 0.5f - halfWidth, 1.0f, yMax, 0.5f + halfWidth);
	else if (side == 3) setBlockBounds(0.5f - halfWidth, yMin, 0.0f, 0.5f + halfWidth, yMax, depth);
	else if (side == 4) setBlockBounds(0.5f - halfWidth, yMin, 1.0f - depth, 0.5f + halfWidth, yMax, 1.0f);
}

void BlockButton::setBlockBoundsBasedOnState(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k)
{
	setButtonBounds(iblockaccess->getBlockMetadata(i, j, k));
}

void BlockButton::onBlockClicked(World *world, int_t i, int_t j, int_t k, EntityPlayer *entityplayer)
{
	blockActivated(world, i, j, k, entityplayer);
}

bool BlockButton::blockActivated(World *world, int_t i, int_t j, int_t k, EntityPlayer *entityplayer)
{
	(void)entityplayer;
	int_t meta = world->getBlockMetadata(i, j, k);
	int_t side = meta & 7;
	int_t toggle = 8 - (meta & 8);
	if (toggle == 0) return true;

	world->setBlockMetadataWithNotify(i, j, k, side + toggle);
	world->markBlocksDirty(i, j, k, i, j, k);
	world->playSoundEffect((double)i + 0.5, (double)j + 0.5, (double)k + 0.5, "random.click", 0.3f, 0.6f);
	notifyNeighbors(world, i, j, k, side);
	world->scheduleBlockUpdate(i, j, k, blockID, tickRate());
	return true;
}

void BlockButton::onBlockRemoval(World *world, int_t i, int_t j, int_t k)
{
	int_t meta = world->getBlockMetadata(i, j, k);
	if ((meta & 8) > 0)
	{
		notifyNeighbors(world, i, j, k, meta & 7);
	}
	Block::onBlockRemoval(world, i, j, k);
}

int_t BlockButton::isProvidingWeakPower(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k, int_t l)
{
	(void)l;
	return (iblockaccess->getBlockMetadata(i, j, k) & 8) > 0 ? 15 : 0;
}

int_t BlockButton::isProvidingStrongPower(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k, int_t l)
{
	int_t meta = iblockaccess->getBlockMetadata(i, j, k);
	if ((meta & 8) == 0) return 0;
	int_t side = meta & 7;
	if (side == 5 && l == 1) return 15;
	if (side == 4 && l == 2) return 15;
	if (side == 3 && l == 3) return 15;
	if (side == 2 && l == 4) return 15;
	if (side == 1 && l == 5) return 15;
	return 0;
}

bool BlockButton::isPoweringTo(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k, int_t l)
{
	return isProvidingWeakPower(iblockaccess, i, j, k, l) > 0;
}

bool BlockButton::isIndirectlyPoweringTo(World *world, int_t i, int_t j, int_t k, int_t l)
{
	return isProvidingStrongPower(world, i, j, k, l) > 0;
}

bool BlockButton::canProvidePower()
{
	return true;
}

void BlockButton::updateTick(World *world, int_t i, int_t j, int_t k, Random &random)
{
	(void)random;
	if (world->multiplayerWorld) return;
	int_t meta = world->getBlockMetadata(i, j, k);
	if ((meta & 8) == 0) return;

	if (sensible)
	{
		checkArrowCollision(world, i, j, k);
	}
	else
	{
		world->setBlockMetadataWithNotify(i, j, k, meta & 7);
		notifyNeighbors(world, i, j, k, meta & 7);
		world->playSoundEffect((double)i + 0.5, (double)j + 0.5, (double)k + 0.5, "random.click", 0.3f, 0.5f);
		world->markBlocksDirty(i, j, k, i, j, k);
	}
}

void BlockButton::onEntityCollidedWithBlock(World *world, int_t i, int_t j, int_t k, Entity *entity)
{
	(void)entity;
	if (!world->multiplayerWorld && sensible)
	{
		if ((world->getBlockMetadata(i, j, k) & 8) == 0)
		{
			checkArrowCollision(world, i, j, k);
		}
	}
}

void BlockButton::checkArrowCollision(World *world, int_t i, int_t j, int_t k)
{
	int_t meta = world->getBlockMetadata(i, j, k);
	int_t side = meta & 7;
	bool pressed = (meta & 8) != 0;
	setButtonBounds(meta);

	AxisAlignedBB *box = AxisAlignedBB::getBoundingBoxFromPool(
		(double)i + minX, (double)j + minY, (double)k + minZ,
		(double)i + maxX, (double)j + maxY, (double)k + maxZ);

	const auto &arrows = world->getEntitiesWithinAABB(typeid(EntityArrow), box);
	bool hasArrow = !arrows.empty();

	if (hasArrow && !pressed)
	{
		world->setBlockMetadataWithNotify(i, j, k, side | 8);
		notifyNeighbors(world, i, j, k, side);
		world->markBlocksDirty(i, j, k, i, j, k);
		world->playSoundEffect((double)i + 0.5, (double)j + 0.5, (double)k + 0.5, "random.click", 0.3f, 0.6f);
	}
	if (!hasArrow && pressed)
	{
		world->setBlockMetadataWithNotify(i, j, k, side);
		notifyNeighbors(world, i, j, k, side);
		world->markBlocksDirty(i, j, k, i, j, k);
		world->playSoundEffect((double)i + 0.5, (double)j + 0.5, (double)k + 0.5, "random.click", 0.3f, 0.5f);
	}
	if (hasArrow)
	{
		world->scheduleBlockUpdate(i, j, k, blockID, tickRate());
	}
}

void BlockButton::notifyNeighbors(World *world, int_t i, int_t j, int_t k, int_t side)
{
	world->notifyBlocksOfNeighborChange(i, j, k, blockID);
	if (side == 1) world->notifyBlocksOfNeighborChange(i - 1, j, k, blockID);
	else if (side == 2) world->notifyBlocksOfNeighborChange(i + 1, j, k, blockID);
	else if (side == 3) world->notifyBlocksOfNeighborChange(i, j, k - 1, blockID);
	else if (side == 4) world->notifyBlocksOfNeighborChange(i, j, k + 1, blockID);
	else world->notifyBlocksOfNeighborChange(i, j - 1, k, blockID);
}

void BlockButton::setBlockBoundsForItemRender()
{
	float f = 0.1875f;
	float f1 = 0.125f;
	float f2 = 0.125f;
	setBlockBounds(0.5f - f, 0.5f - f1, 0.5f - f2, 0.5f + f, 0.5f + f1, 0.5f + f2);
}
