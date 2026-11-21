#include "BlockGravel.h"
#include "Item.h"
#include "java/Random.h"

BlockGravel::BlockGravel(int_t i, int_t j) : BlockSand(i, j)
{
}

int_t BlockGravel::idDropped(int_t i, Random &random)
{
    if (random.nextInt(10) == 0)
        return Item::flint->shiftedIndex;
    return blockID;
}

int_t BlockGravel::idDropped(int_t i, Random &random, int_t fortune)
{
    (void)i;
    if (fortune > 3)
        fortune = 3;
    int_t bound = 10 - fortune * 3;
    if (bound < 1)
        bound = 1;
    return random.nextInt(bound) == 0 ? Item::flint->shiftedIndex : blockID;
}
