#include "BlockCarrot.h"
#include "Texture152Atlas.h"

#include "Item.h"

BlockCarrot::BlockCarrot(int_t i) : BlockCrops(i, TEX152_BLOCK_ICON("carrots_0.png")) // 1.5.2 icon: carrots_0.png
{
}

int_t BlockCarrot::getBlockTextureFromSideAndMetadata(int_t i, int_t j)
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
			return TEX152_BLOCK_ICON("carrots_0.png"); // 1.5.2 icon: carrots_0.png
		case 1:
			return TEX152_BLOCK_ICON("carrots_1.png"); // 1.5.2 icon: carrots_1.png
		default:
			return TEX152_BLOCK_ICON("carrots_2.png"); // 1.5.2 icon: carrots_2.png
		}
	}
	return TEX152_BLOCK_ICON("carrots_3.png"); // 1.5.2 icon: carrots_3.png
}

int_t BlockCarrot::getSeedItem()
{
	return Item::carrot->shiftedIndex;
}

int_t BlockCarrot::getCropItem()
{
	return Item::carrot->shiftedIndex;
}
