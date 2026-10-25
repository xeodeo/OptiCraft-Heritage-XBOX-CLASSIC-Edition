#include "BlockRailBase.h"
#include "Block.h"
#include "World.h"

bool BlockRailBase::isRailBlockAt(World *world, int_t x, int_t y, int_t z)
{
    return isRailBlock(world->getBlockId(x, y, z));
}

bool BlockRailBase::isRailBlock(int_t blockId)
{
    return blockId == Block::rail->blockID || blockId == Block::railPowered->blockID ||
           blockId == Block::railDetector->blockID ||
           (Block::railActivator != nullptr && blockId == Block::railActivator->blockID);
}
