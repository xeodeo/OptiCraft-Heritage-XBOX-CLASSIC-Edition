#include "BlockCarrot.h"

#include "Item.h"

BlockCarrot::BlockCarrot(int_t i) : BlockCrops(i, 88) // PORT152_ICON: carrots_0
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
			return 88; // PORT152_ICON: carrots_0
		case 1:
			return 90; // PORT152_ICON: carrots_1
		default:
			return 92; // PORT152_ICON: carrots_2
		}
	}
	return 95; // PORT152_ICON: carrots_3
}

int_t BlockCarrot::getSeedItem()
{
	return Item::carrot->shiftedIndex;
}

int_t BlockCarrot::getCropItem()
{
	return Item::carrot->shiftedIndex;
}
