#pragma once

#include "BlockCrops.h"

// net.minecraft.src.BlockPotato
class BlockPotato : public BlockCrops
{
public:
	explicit BlockPotato(int_t i);
	int_t getBlockTextureFromSideAndMetadata(int_t i, int_t j) override;
	void dropBlockAsItemWithChance(World *world, int_t i, int_t j, int_t k, int_t l, float f, int_t i1) override;

protected:
	int_t getSeedItem() override;
	int_t getCropItem() override;
};
