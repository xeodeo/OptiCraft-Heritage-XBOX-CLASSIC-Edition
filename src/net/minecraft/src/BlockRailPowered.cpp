#include "BlockRailPowered.h"
#include "Texture152Atlas.h"

BlockRailPowered::BlockRailPowered(int_t id, bool activator)
    : BlockRail(id, activator ? TEX152_BLOCK_ICON("activatorRail.png") : TEX152_BLOCK_ICON("goldenRail.png"), true),
      activatorRail(activator)
{
}

int_t BlockRailPowered::getBlockTextureFromSideAndMetadata(int_t side, int_t metadata)
{
    (void)side;
    if ((metadata & 8) == 0)
        return blockIndexInTexture;
    return activatorRail ? TEX152_BLOCK_ICON("activatorRail_powered.png") : TEX152_BLOCK_ICON("goldenRail_powered.png");
}
