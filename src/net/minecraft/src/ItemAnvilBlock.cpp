#include "ItemAnvilBlock.h"
#include "BlockAnvil.h"

ItemAnvilBlock::ItemAnvilBlock(Block *block)
	: ItemMultiTextureTile(block != nullptr ? block->blockID - 256 : 0, block, BlockAnvil::statuses, 3)
{
}

ItemAnvilBlock::ItemAnvilBlock(int_t id, Block *block)
	: ItemMultiTextureTile(id, block, BlockAnvil::statuses, 3)
{
}
