#include "BlockButtonWood.h"
#include "Block.h"

BlockButtonWood::BlockButtonWood(int_t id)
	: BlockButton(id, true)
{
}

int_t BlockButtonWood::getBlockTextureFromSide(int_t side)
{
	(void)side;
	return Block::planks != nullptr ? Block::planks->getBlockTextureFromSide(1) : 4;
}
