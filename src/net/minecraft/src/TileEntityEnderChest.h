#pragma once

#include "TileEntity.h"

class EntityPlayer;

// net.minecraft.src.TileEntityEnderChest
class TileEntityEnderChest : public TileEntity
{
public:
	float lidAngle = 0.0f;
	float prevLidAngle = 0.0f;
	int_t numUsingPlayers = 0;

	TileEntityEnderChest();
	~TileEntityEnderChest() override = default;

	void updateEntity() override;
	void onTileEntityPowered(int_t eventId, int_t eventData) override;
	void openChest();
	void closeChest();
	bool isUseableByPlayer(EntityPlayer *entityplayer);
	void invalidate() override;

private:
	int_t ticksSinceSync = 0;
};
