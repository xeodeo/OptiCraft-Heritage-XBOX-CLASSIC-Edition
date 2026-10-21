#pragma once

#include "TileEntitySpecialRenderer.h"

class ModelChest;
class TileEntityEnderChest;

// net.minecraft.src.TileEntityEnderChestRenderer
class TileEntityEnderChestRenderer : public TileEntitySpecialRenderer
{
public:
	TileEntityEnderChestRenderer();
	~TileEntityEnderChestRenderer() override;

	void renderTileEntityAt(TileEntity *tileentity, double d, double d1, double d2, float f) override;

private:
	ModelChest *theEnderChestModel;
};
