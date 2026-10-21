#pragma once

#include "BlockContainer.h"

// net.minecraft.src.BlockBeacon
class BlockBeacon : public BlockContainer
{
public:
	explicit BlockBeacon(int_t id);

	TileEntity *createNewTileEntity(World *world);
	TileEntity *getBlockEntity() override;
	bool blockActivated(World *world, int_t i, int_t j, int_t k, EntityPlayer *entityplayer) override;
	bool isOpaqueCube() override { return false; }
	bool renderAsNormalBlock() override { return false; }
	int_t getRenderType() override { return 34; }
	int_t getBeaconIcon() const;
};
