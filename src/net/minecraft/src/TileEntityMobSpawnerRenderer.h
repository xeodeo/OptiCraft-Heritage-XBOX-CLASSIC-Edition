#pragma once

#include "TileEntitySpecialRenderer.h"

class MobSpawnerBaseLogic;
class TileEntityMobSpawner;

class TileEntityMobSpawnerRenderer : public TileEntitySpecialRenderer
{
public:
	TileEntityMobSpawnerRenderer();
	~TileEntityMobSpawnerRenderer() override;

	static void renderMobSpawnerLogic(MobSpawnerBaseLogic *logic, double d, double d1, double d2, float f);
	void renderTileEntityMobSpawner(TileEntityMobSpawner *tileentitymobspawner, double d, double d1, double d2, float f);
	void renderTileEntityAt(TileEntity *tileentity, double d, double d1, double d2, float f) override;
};
