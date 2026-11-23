#include "BlockLog.h"
#include "ItemStack.h"
#include "Material.h"
#include "World.h"
#include "Block.h"
#include "EntityPlayer.h"
#include "BlockLeaves.h"

BlockLog::BlockLog(int_t i) : Block(i, Material::wood)
{
	blockIndexInTexture = 20;
}

int_t BlockLog::quantityDropped(Random &random) { return 1; }

int_t BlockLog::idDropped(int_t i, Random &random)
{
	return Block::wood->blockID;
}

void BlockLog::harvestBlock(World *world, EntityPlayer *entityplayer, int_t i, int_t j, int_t k, int_t l)
{
	Block::harvestBlock(world, entityplayer, i, j, k, l);
}

void BlockLog::onBlockRemoval(World *world, int_t i, int_t j, int_t k)
{
	byte_t byte0 = 4;
	int_t l = byte0 + 1;
	if (world->checkChunksExist(i - l, j - l, k - l, i + l, j + l, k + l))
	{
		for (int_t i1 = -byte0; i1 <= byte0; i1++)
		{
			for (int_t j1 = -byte0; j1 <= byte0; j1++)
			{
				for (int_t k1 = -byte0; k1 <= byte0; k1++)
				{
					int_t l1 = world->getBlockId(i + i1, j + j1, k + k1);
					if (l1 != Block::leaves->blockID) continue;
					int_t i2 = world->getBlockMetadata(i + i1, j + j1, k + k1);
					if ((i2 & 8) == 0)
					{
						world->setBlockMetadata(i + i1, j + j1, k + k1, i2 | 8);
					}
				}
			}
		}
	}
}

int_t BlockLog::onBlockPlaced(World *world, int_t i, int_t j, int_t k, int_t side, float hitX, float hitY, float hitZ, int_t metadata)
{
	(void)world; (void)i; (void)j; (void)k; (void)hitX; (void)hitY; (void)hitZ;
	int_t type = metadata & 3;
	int_t dir = 0;
	switch (side)
	{
	case 0:
	case 1:
		dir = 0;
		break;
	case 2:
	case 3:
		dir = 8;
		break;
	case 4:
	case 5:
		dir = 4;
		break;
	}
	return type | dir;
}

void BlockLog::onBlockPlaced(World *world, int_t i, int_t j, int_t k, int_t side)
{
	int_t meta = onBlockPlaced(world, i, j, k, side, 0.0f, 0.0f, 0.0f, world->getBlockMetadata(i, j, k));
	world->setBlockMetadataWithNotify(i, j, k, meta);
}

int_t BlockLog::getBlockTextureFromSideAndMetadata(int_t i, int_t j)
{
	const int_t dir = j & 12;
	const int_t type = j & 3;
	if ((dir == 0 && (i == 1 || i == 0)) ||
	    (dir == 4 && (i == 5 || i == 4)) ||
	    (dir == 8 && (i == 2 || i == 3)))
	{
		return 21;
	}
	if (type == 1) return 116;
	if (type == 2) return 117;
	if (type == 3) return 153;
	return 20;
}

int_t BlockLog::damageDropped(int_t i) { return i & 3; }

ItemStack *BlockLog::createStackedBlock(int_t metadata)
{
	return new ItemStack(blockID, 1, limitToValidMetadata(metadata));
}

void BlockLog::getSubBlocks(int_t id, CreativeTabs *tab, std::vector<ItemStack *> &list)
{
	(void)tab;
	for (int_t i = 0; i < 4; ++i)
		list.push_back(new ItemStack(id, 1, i));
}
