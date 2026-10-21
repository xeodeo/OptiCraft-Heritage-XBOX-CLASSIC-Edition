#include "BlockQuartz.h"
#include "Texture152Atlas.h"

#include "ItemStack.h"
#include "Material.h"
#include "World.h"

const char *const BlockQuartz::quartzBlockTypes[3] = {
	"default",
	"chiseled",
	"lines"
};

BlockQuartz::BlockQuartz(int_t i) : Block(i, TEX152_BLOCK_ICON("quartzblock_side.png"), Material::rock) // 1.5.2 icon: quartzblock_side.png
{
}

int_t BlockQuartz::getBlockTextureFromSideAndMetadata(int_t side, int_t metadata)
{
	if (metadata == 1)
	{
		if (side == 1 || side == 0)
		{
			return TEX152_BLOCK_ICON("quartzblock_chiseled_top.png"); // 1.5.2 icon: quartzblock_chiseled_top.png
		}
		return TEX152_BLOCK_ICON("quartzblock_chiseled.png"); // 1.5.2 icon: quartzblock_chiseled.png
	}
	if (metadata == 2 || metadata == 3 || metadata == 4)
	{
		if ((metadata == 2 && (side == 1 || side == 0)) ||
		    (metadata == 3 && (side == 5 || side == 4)) ||
		    (metadata == 4 && (side == 2 || side == 3)))
		{
			return TEX152_BLOCK_ICON("quartzblock_lines_top.png"); // 1.5.2 icon: quartzblock_lines_top.png
		}
		return TEX152_BLOCK_ICON("quartzblock_lines.png"); // 1.5.2 icon: quartzblock_lines.png
	}
	if (side == 0)
	{
		return TEX152_BLOCK_ICON("quartzblock_bottom.png"); // 1.5.2 icon: quartzblock_bottom.png
	}
	if (side == 1)
	{
		return TEX152_BLOCK_ICON("quartzblock_top.png"); // 1.5.2 icon: quartzblock_top.png
	}
	return TEX152_BLOCK_ICON("quartzblock_side.png"); // 1.5.2 icon: quartzblock_side.png
}

int_t BlockQuartz::getBlockTextureFromSide(int_t side)
{
	return getBlockTextureFromSideAndMetadata(side, 0);
}

int_t BlockQuartz::onBlockPlaced(World *world, int_t i, int_t j, int_t k, int_t side, float hitX, float hitY, float hitZ, int_t metadata)
{
	(void)world; (void)i; (void)j; (void)k; (void)hitX; (void)hitY; (void)hitZ;
	if (metadata == 2)
	{
		switch (side)
		{
		case 0:
		case 1:
			metadata = 2;
			break;
		case 2:
		case 3:
			metadata = 4;
			break;
		case 4:
		case 5:
			metadata = 3;
			break;
		}
	}
	return metadata;
}

void BlockQuartz::onBlockPlaced(World *world, int_t i, int_t j, int_t k, int_t side)
{
	int_t meta = onBlockPlaced(world, i, j, k, side, 0.0f, 0.0f, 0.0f, world->getBlockMetadata(i, j, k));
	world->setBlockMetadataWithNotify(i, j, k, meta);
}

int_t BlockQuartz::damageDropped(int_t metadata)
{
	return metadata != 3 && metadata != 4 ? metadata : 2;
}

ItemStack *BlockQuartz::createStackedBlock(int_t metadata)
{
	return metadata != 3 && metadata != 4 ? Block::createStackedBlock(metadata) : new ItemStack(blockID, 1, 2);
}
