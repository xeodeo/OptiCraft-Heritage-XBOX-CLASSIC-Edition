#include "BlockQuartz.h"

#include "ItemStack.h"
#include "Material.h"

const char *const BlockQuartz::quartzBlockTypes[3] = {
	"default",
	"chiseled",
	"lines"
};

BlockQuartz::BlockQuartz(int_t i) : Block(i, 192, Material::rock) // PORT152_ICON: quartzblock_side
{
}

int_t BlockQuartz::getBlockTextureFromSideAndMetadata(int_t side, int_t metadata)
{
	if (metadata == 1)
	{
		if (side == 1 || side == 0)
		{
			return 176; // PORT152_ICON: quartzblock_chiseled_top
		}
		return 229; // PORT152_ICON: quartzblock_chiseled
	}
	if (metadata == 2 || metadata == 3 || metadata == 4)
	{
		if ((metadata == 2 && (side == 1 || side == 0)) ||
		    (metadata == 3 && (side == 5 || side == 4)) ||
		    (metadata == 4 && (side == 2 || side == 3)))
		{
			return 176; // PORT152_ICON: quartzblock_lines_top
		}
		return 230; // PORT152_ICON: quartzblock_lines
	}
	if (side == 0)
	{
		return 208; // PORT152_ICON: quartzblock_bottom
	}
	if (side == 1)
	{
		return 176; // PORT152_ICON: quartzblock_top
	}
	return 192; // PORT152_ICON: quartzblock_side
}

int_t BlockQuartz::getBlockTextureFromSide(int_t side)
{
	return getBlockTextureFromSideAndMetadata(side, 0);
}

int_t BlockQuartz::damageDropped(int_t metadata)
{
	return metadata != 3 && metadata != 4 ? metadata : 2;
}

ItemStack *BlockQuartz::createStackedBlock(int_t metadata)
{
	return metadata != 3 && metadata != 4 ? Block::createStackedBlock(metadata) : new ItemStack(blockID, 1, 2);
}
