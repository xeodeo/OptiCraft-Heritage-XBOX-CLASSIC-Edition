#pragma once

#include "TileEntitySpecialRenderer.h"
#include "ModelSkeletonHead.h"
#include <string>

class TileEntitySkull;

// net.minecraft.src.TileEntitySkullRenderer
class TileEntitySkullRenderer : public TileEntitySpecialRenderer
{
public:
	static TileEntitySkullRenderer *skullRenderer;

	TileEntitySkullRenderer();
	~TileEntitySkullRenderer() override;

	void renderTileEntityAt(TileEntity *tileentity, double d, double d1, double d2, float f) override;
	void renderSkull(float x, float y, float z, int_t side, float rotation, int_t type, const std::string &extra);

private:
	ModelSkeletonHead field_82396_c;
	ModelSkeletonHead field_82395_d;
};
