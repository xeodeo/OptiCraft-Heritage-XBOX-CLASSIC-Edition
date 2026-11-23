#pragma once

#include "BlockFlower.h"

class IBlockAccess;

// net.minecraft.src.BlockTallGrass
class BlockTallGrass : public BlockFlower
{
public:
	BlockTallGrass(int_t i, int_t j);
	int_t getBlockTextureFromSideAndMetadata(int_t i, int_t j) override;
	int_t getBlockColor() override;
	int_t getRenderColor(int_t i) override;
	int_t colorMultiplier(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k) override;
	bool usesDefaultColorMultiplier() const override { return false; }
	int_t idDropped(int_t i, Random &random) override;
	int_t quantityDroppedWithBonus(int_t fortune, Random &random) override;
	void getSubBlocks(int_t id, CreativeTabs *tab, std::vector<ItemStack *> &list) override;
	void harvestBlock(World *world, EntityPlayer *entityplayer, int_t i, int_t j, int_t k, int_t metadata) override;

	// Not in Java (Forge addGrassSeed): when tall grass drops a seed, a mod may pick
	// its own seed item id instead (> 0), or return 0 for wheat seeds.
	using GrassSeedHook = int_t (*)(Random &random);
	static GrassSeedHook grassSeedHook;
};
