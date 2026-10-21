#pragma once

#include "BlockCrops.h"

// net.minecraft.src.BlockCarrot
class BlockCarrot : public BlockCrops
{
public:
	explicit BlockCarrot(int_t i);
	int_t getBlockTextureFromSideAndMetadata(int_t i, int_t j) override;

protected:
	int_t getSeedItem() override;
	int_t getCropItem() override;
};
