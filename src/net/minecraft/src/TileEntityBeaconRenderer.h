#pragma once

#include "TileEntitySpecialRenderer.h"

class TileEntityBeacon;

// net.minecraft.src.TileEntityBeaconRenderer
class TileEntityBeaconRenderer : public TileEntitySpecialRenderer
{
public:
	TileEntityBeaconRenderer();
	~TileEntityBeaconRenderer() override;

	void renderTileEntityBeaconAt(TileEntityBeacon *beacon, double x, double y, double z, float partialTick);
	void renderTileEntityAt(TileEntity *tileentity, double x, double y, double z, float partialTick) override;
};
