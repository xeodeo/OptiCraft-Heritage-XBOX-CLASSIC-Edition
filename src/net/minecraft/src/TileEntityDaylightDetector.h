#pragma once
#include "TileEntity.h"

// net.minecraft.src.TileEntityDaylightDetector
class TileEntityDaylightDetector : public TileEntity
{
public:
	void updateEntity() override;
};
