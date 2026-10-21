#include "BlockPotato.h"

#include "Item.h"
#include "ItemStack.h"
#include "World.h"

BlockPotato::BlockPotato(int_t i) : BlockCrops(i, 88) // PORT152_ICON: potatoes_0
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
			return 88; // PORT152_ICON: potatoes_0
		case 1:
			return 90; // PORT152_ICON: potatoes_1
		default:
			return 92; // PORT152_ICON: potatoes_2
		}
	}
	return 95; // PORT152_ICON: potatoes_3
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
