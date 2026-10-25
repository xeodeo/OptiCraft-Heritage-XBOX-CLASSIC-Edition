#pragma once
#include "BlockRail.h"

// 1.5.2 powered and activator rail.  The legacy movement and propagation code
// already lives in BlockRail; this class supplies the 1.5.2 icon selection.
class BlockRailPowered : public BlockRail
{
public:
    BlockRailPowered(int_t id, bool activator = false);
    int_t getBlockTextureFromSideAndMetadata(int_t side, int_t metadata) override;

private:
    bool activatorRail;
};
