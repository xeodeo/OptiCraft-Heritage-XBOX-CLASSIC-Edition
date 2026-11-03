#include "BlockWoodSlab.h"

#include "Block.h"
#include "ItemStack.h"
#include "Material.h"

const char *const BlockWoodSlab::woodType[4] = {"oak", "spruce", "birch", "jungle"};

BlockWoodSlab::BlockWoodSlab(int_t id, bool isDouble)
	: BlockHalfSlab(id, isDouble, Material::wood)
{
}

int_t BlockWoodSlab::getBlockTextureFromSideAndMetadata(int_t side, int_t metadata)
{
	return Block::planks->getBlockTextureFromSideAndMetadata(side, metadata & 7);
}

int_t BlockWoodSlab::getBlockTextureFromSide(int_t side)
{
	return getBlockTextureFromSideAndMetadata(side, 0);
}

int_t BlockWoodSlab::idDropped(int_t i, Random &random)
{
	(void)i;
	(void)random;
	return Block::woodSingleSlab->blockID;
}

std::string BlockWoodSlab::getFullSlabName(int_t metadata)
{
	int_t damage = metadata & 7;
	if (damage < 0 || damage >= 4)
	{
		damage = 0;
	}
	return getBlockName() + "." + woodType[damage];
}

ItemStack *BlockWoodSlab::createStackedBlock(int_t metadata)
{
	return new ItemStack(Block::woodSingleSlab->blockID, 2, metadata & 7);
}

void BlockWoodSlab::getSubBlocks(int_t id, CreativeTabs *tab, std::vector<ItemStack *> &list)
{
	(void)tab;
	if (Block::woodDoubleSlab != nullptr && id != Block::woodDoubleSlab->blockID)
	{
		for (int_t i = 0; i < 4; ++i)
		{
			list.push_back(new ItemStack(id, 1, i));
		}
	}
}
