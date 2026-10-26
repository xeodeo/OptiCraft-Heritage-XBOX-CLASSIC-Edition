#pragma once

#include "TileEntityDispenser.h"
#include <string>

// net.minecraft.src.TileEntityDropper
class TileEntityDropper : public TileEntityDispenser
{
public:
	TileEntityDropper();
	std::string getInvName() override;
};
