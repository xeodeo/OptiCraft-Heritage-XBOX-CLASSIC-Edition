#pragma once
#include "java/Type.h"

// Shared 1.5.2 rail API.  BlockRail predates this name in the C++ port; the
// concrete rails retain that implementation while new callers use this helper.
class World;
class BlockRailBase
{
public:
    static bool isRailBlockAt(World *world, int_t x, int_t y, int_t z);
    static bool isRailBlock(int_t blockId);
};
