#include "BlockComparator.h"
#include "TileEntityComparator.h"
#include "World.h"
#include "IBlockAccess.h"
#include "Block.h"
#include "Item.h"
#include "Direction.h"
#include "EntityPlayer.h"
#include "Texture152Atlas.h"
#include <algorithm>

BlockComparator::BlockComparator(int_t id, bool isPowered)
	: BlockRedstoneLogic(id, isPowered)
{
	isBlockContainer[id] = true;
	setRequiresSelfNotify();
}

int_t BlockComparator::idDropped(int_t metadata, Random &random)
{
	(void)metadata; (void)random;
	return Item::comparator != nullptr ? Item::comparator->shiftedIndex : 0;
}

int_t BlockComparator::getRenderType()
{
	return 37;
}

int_t BlockComparator::getBlockTextureFromSideAndMetadata(int_t i, int_t j)
{
	bool powered = isRepeaterPowered || (j & 8) != 0;
	if (i == 0)
	{
		return powered ? Block::torchRedstoneActive->getBlockTextureFromSide(0) : Block::torchRedstoneIdle->getBlockTextureFromSide(0);
	}
	else if (i == 1)
	{
		return powered ? TEX152_BLOCK_ICON("comparator_lit.png") : TEX152_BLOCK_ICON("comparator.png");
	}
	else
	{
		return Block::stairDouble != nullptr ? Block::stairDouble->getBlockTextureFromSide(1) : 6;
	}
}

int_t BlockComparator::getBlockTextureFromSide(int_t i)
{
	return getBlockTextureFromSideAndMetadata(i, 0);
}

bool BlockComparator::func_96470_c(int_t metadata)
{
	return isRepeaterPowered || (metadata & 8) != 0;
}

int_t BlockComparator::func_94480_d(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k, int_t l)
{
	(void)l;
	TileEntityComparator *te = getTileEntityComparator(iblockaccess, i, j, k);
	return te != nullptr ? te->func_96100_a() : 0;
}

int_t BlockComparator::func_94491_m(World *world, int_t i, int_t j, int_t k, int_t l)
{
	if (!func_94490_c(l))
	{
		return getInputStrength(world, i, j, k, l);
	}
	return std::max(getInputStrength(world, i, j, k, l) - func_94482_f(world, i, j, k, l), 0);
}

bool BlockComparator::func_94490_c(int_t metadata) const
{
	return (metadata & 4) == 4;
}

bool BlockComparator::func_94478_d(World *world, int_t i, int_t j, int_t k, int_t l)
{
	int_t input = getInputStrength(world, i, j, k, l);
	if (input >= 15)
	{
		return true;
	}
	if (input == 0)
	{
		return false;
	}
	int_t side = func_94482_f(world, i, j, k, l);
	return side == 0 ? true : input >= side;
}

int_t BlockComparator::getInputStrength(World *world, int_t i, int_t j, int_t k, int_t l)
{
	int_t strength = BlockRedstoneLogic::getInputStrength(world, i, j, k, l);
	int_t dir = getDirection(l);
	int_t nx = i + Direction::offsetX[dir];
	int_t nz = k + Direction::offsetZ[dir];
	int_t blockId = world->getBlockId(nx, j, nz);
	if (blockId > 0 && blockId < Block::BLOCK_REGISTRY_SIZE && Block::blocksList[blockId] != nullptr)
	{
		if (Block::blocksList[blockId]->hasComparatorInputOverride())
		{
			strength = Block::blocksList[blockId]->getComparatorInputOverride(world, nx, j, nz, Direction::rotateOpposite[dir]);
		}
		else if (strength < 15 && Block::isNormalCube(blockId))
		{
			nx += Direction::offsetX[dir];
			nz += Direction::offsetZ[dir];
			blockId = world->getBlockId(nx, j, nz);
			if (blockId > 0 && blockId < Block::BLOCK_REGISTRY_SIZE && Block::blocksList[blockId] != nullptr &&
				Block::blocksList[blockId]->hasComparatorInputOverride())
			{
				strength = Block::blocksList[blockId]->getComparatorInputOverride(world, nx, j, nz, Direction::rotateOpposite[dir]);
			}
		}
	}
	return strength;
}

TileEntityComparator *BlockComparator::getTileEntityComparator(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k)
{
	return dynamic_cast<TileEntityComparator *>(iblockaccess->getBlockTileEntity(i, j, k));
}

bool BlockComparator::blockActivated(World *world, int_t i, int_t j, int_t k, EntityPlayer *entityplayer)
{
	(void)entityplayer;
	int_t meta = world->getBlockMetadata(i, j, k);
	bool powered = isRepeaterPowered || (meta & 8) != 0;
	bool subtractMode = !func_94490_c(meta);
	int_t newMeta = subtractMode ? 4 : 0;
	newMeta |= powered ? 8 : 0;
	world->playSoundEffect((double)i + 0.5, (double)j + 0.5, (double)k + 0.5, "random.click", 0.3f, subtractMode ? 0.55f : 0.5f);
	world->setBlockMetadataWithNotify152(i, j, k, newMeta | (meta & 3), 2);
	func_96476_c(world, i, j, k, world->rand);
	return true;
}

void BlockComparator::func_94479_f(World *world, int_t i, int_t j, int_t k, int_t l)
{
	(void)l;
	if (!world->isBlockTickScheduled(i, j, k, blockID))
	{
		int_t meta = world->getBlockMetadata(i, j, k);
		int_t desiredOutput = func_94491_m(world, i, j, k, meta);
		TileEntityComparator *te = getTileEntityComparator(world, i, j, k);
		int_t currentOutput = te != nullptr ? te->func_96100_a() : 0;
		if (desiredOutput != currentOutput || func_96470_c(meta) != func_94478_d(world, i, j, k, meta))
		{
			int_t priority = func_83011_d(world, i, j, k, meta) ? -1 : 0;
			world->func_82740_a(i, j, k, blockID, func_94481_j_(0), priority);
		}
	}
}

void BlockComparator::func_96476_c(World *world, int_t i, int_t j, int_t k, Random &random)
{
	(void)random;
	int_t meta = world->getBlockMetadata(i, j, k);
	int_t desiredOutput = func_94491_m(world, i, j, k, meta);
	TileEntityComparator *te = getTileEntityComparator(world, i, j, k);
	int_t currentOutput = te != nullptr ? te->func_96100_a() : 0;
	if (te != nullptr)
	{
		te->func_96099_a(desiredOutput);
	}
	if (currentOutput != desiredOutput || !func_94490_c(meta))
	{
		bool shouldBePowered = func_94478_d(world, i, j, k, meta);
		bool currentlyPowered = isRepeaterPowered || (meta & 8) != 0;
		if (currentlyPowered && !shouldBePowered)
		{
			world->setBlockMetadataWithNotify152(i, j, k, meta & ~8, 2);
		}
		else if (!currentlyPowered && shouldBePowered)
		{
			world->setBlockMetadataWithNotify152(i, j, k, meta | 8, 2);
		}
		func_94483_i_(world, i, j, k);
	}
}

void BlockComparator::updateTick(World *world, int_t i, int_t j, int_t k, Random &random)
{
	if (isRepeaterPowered)
	{
		int_t meta = world->getBlockMetadata(i, j, k);
		world->setBlockNotify152(i, j, k, func_94484_i()->blockID, meta | 8, 4);
	}
	func_96476_c(world, i, j, k, random);
}

void BlockComparator::onBlockAdded(World *world, int_t i, int_t j, int_t k)
{
	BlockRedstoneLogic::onBlockAdded(world, i, j, k);
	world->setBlockTileEntity(i, j, k, getBlockEntity());
}

void BlockComparator::onBlockRemoval(World *world, int_t i, int_t j, int_t k)
{
	BlockRedstoneLogic::onBlockRemoval(world, i, j, k);
	world->removeBlockTileEntity(i, j, k);
	func_94483_i_(world, i, j, k);
}

TileEntity *BlockComparator::getBlockEntity()
{
	return new TileEntityComparator();
}

int_t BlockComparator::func_94481_j_(int_t metadata)
{
	(void)metadata;
	return 2;
}

BlockRedstoneLogic *BlockComparator::func_94485_e()
{
	return dynamic_cast<BlockRedstoneLogic *>(Block::redstoneComparatorActive);
}

BlockRedstoneLogic *BlockComparator::func_94484_i()
{
	return dynamic_cast<BlockRedstoneLogic *>(Block::redstoneComparatorIdle);
}
