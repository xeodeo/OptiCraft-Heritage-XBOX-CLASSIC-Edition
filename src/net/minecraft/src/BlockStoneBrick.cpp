#include "BlockStoneBrick.h"
#include "ItemStack.h"

#include "Material.h"

BlockStoneBrick::BlockStoneBrick(int_t id) :
	Block(id, 54, Material::rock)
{
}

int_t BlockStoneBrick::getBlockTextureFromSideAndMetadata(int_t side, int_t metadata)
{
	switch (metadata)
	{
	case 1: return 100;
	case 2: return 101;
	case 3: return 213;
	default: return 54;
	}
}

void BlockStoneBrick::getSubBlocks(int_t id, CreativeTabs *tab, std::vector<ItemStack *> &list)
{
	(void)tab;
	for (int_t i = 0; i < 4; ++i)
		list.push_back(new ItemStack(id, 1, i));
}

int_t BlockStoneBrick::damageDropped(int_t metadata)
{
	return metadata;
}
