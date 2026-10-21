#pragma once

#include "BlockButton.h"

// net.minecraft.src.BlockButtonWood
class BlockButtonWood : public BlockButton
{
public:
	explicit BlockButtonWood(int_t id);

	int_t getBlockTextureFromSide(int_t side) override;
};
