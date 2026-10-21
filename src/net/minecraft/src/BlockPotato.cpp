#include "BlockPotato.h"
#include "Texture152Atlas.h"

#include "Item.h"
#include "ItemStack.h"
#include "World.h"

BlockPotato::BlockPotato(int_t i) : BlockCrops(i, TEX152_BLOCK_ICON("potatoes_0.png")) // 1.5.2 icon: potatoes_0.png
{
}

int_t BlockPotato::getBlockTextureFromSideAndMetadata(int_t i, int_t j)
{
	if (j < 7)
	{
		if (j == 6)
		{
			j = 5;
		}
		switch (j >> 1)
		{
		case 0:
			return TEX152_BLOCK_ICON("potatoes_0.png"); // 1.5.2 icon: potatoes_0.png
		case 1:
			return TEX152_BLOCK_ICON("potatoes_1.png"); // 1.5.2 icon: potatoes_1.png
		default:
			return TEX152_BLOCK_ICON("potatoes_2.png"); // 1.5.2 icon: potatoes_2.png
		}
	}
	return TEX152_BLOCK_ICON("potatoes_3.png"); // 1.5.2 icon: potatoes_3.png
}

int_t BlockPotato::getSeedItem()
{
	return Item::potato->shiftedIndex;
}

int_t BlockPotato::getCropItem()
{
	return Item::potato->shiftedIndex;
}

void BlockPotato::dropBlockAsItemWithChance(World *world, int_t i, int_t j, int_t k, int_t l, float f, int_t i1)
{
	BlockCrops::dropBlockAsItemWithChance(world, i, j, k, l, f, 0);
	if (!world->multiplayerWorld)
	{
		if (l >= 7 && world->rand.nextInt(50) == 0)
		{
			dropBlockAsItem_do(world, i, j, k, new ItemStack(Item::poisonousPotato));
		}
	}
}
