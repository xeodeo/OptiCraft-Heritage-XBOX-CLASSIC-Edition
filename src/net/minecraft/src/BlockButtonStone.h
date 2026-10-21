#pragma once

#include "BlockButton.h"

// net.minecraft.src.BlockButtonStone
class BlockButtonStone : public BlockButton
{
public:
	explicit BlockButtonStone(int_t id);

	int_t getBlockTextureFromSide(int_t side) override;
};
