#include "BlockStep.h"
#include "Texture152Atlas.h"

#include "Block.h"
#include "ItemStack.h"
#include "Material.h"

const jstring BlockStep::blockStepTypes[8] = { "stone", "sand", "wood", "cobble", "brick", "smoothStoneBrick", "netherBrick", "quartz" };

BlockStep::BlockStep(int_t i, bool flag) : BlockHalfSlab(i, flag, Material::rock)
{
}

int_t BlockStep::getBlockTextureFromSideAndMetadata(int_t side, int_t metadata)
{
	const int_t type = metadata & 7;
	if (type == 0) return side <= 1 ? 6 : 5;
	if (type == 1) return side == 0 ? 208 : (side == 1 ? 176 : 192);
	if (type == 2) return 4;
	if (type == 3) return 16;
	if (type == 4) return Block::brick->blockIndexInTexture;
	if (type == 5) return Block::stoneBrick->blockIndexInTexture;
	if (type == 6) return Block::netherBrick->blockIndexInTexture;
	if (type == 7) return side == 0 ? TEX152_BLOCK_ICON("quartzblock_bottom.png") : (side == 1 ? TEX152_BLOCK_ICON("quartzblock_top.png") : TEX152_BLOCK_ICON("quartzblock_side.png")); // 1.5.2 icon: quartzblock_bottom.png, quartzblock_top.png, quartzblock_side.png
	return 6;
}

int_t BlockStep::getBlockTextureFromSide(int_t i)
{
	return getBlockTextureFromSideAndMetadata(i, 0);
}

int_t BlockStep::idDropped(int_t i, Random &random)
{
	(void)i;
	(void)random;
	return Block::stairSingle->blockID;
}

std::string BlockStep::getFullSlabName(int_t metadata)
{
	int_t damage = metadata & 7;
	if (damage < 0 || damage >= 8)
	{
		damage = 0;
	}
	return getBlockName() + "." + blockStepTypes[damage];
}

ItemStack *BlockStep::createStackedBlock(int_t metadata)
{
	return new ItemStack(Block::stairSingle, 1, metadata & 7);
}

void BlockStep::getSubBlocks(int_t id, CreativeTabs *tab, std::vector<ItemStack *> &list)
{
	(void)tab;
	if (Block::stairDouble != nullptr && id != Block::stairDouble->blockID)
	{
		for (int_t i = 0; i <= 7; ++i)
		{
			if (i != 2)
				list.push_back(new ItemStack(id, 1, i));
		}
	}
}
