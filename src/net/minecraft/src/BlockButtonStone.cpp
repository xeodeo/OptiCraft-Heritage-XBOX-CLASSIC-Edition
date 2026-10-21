#include "BlockButtonStone.h"
#include "Block.h"

BlockButtonStone::BlockButtonStone(int_t id)
	: BlockButton(id, false)
{
}

int_t BlockButtonStone::getBlockTextureFromSide(int_t side)
{
	(void)side;
	return Block::stone != nullptr ? Block::stone->getBlockTextureFromSide(1) : 1;
}
