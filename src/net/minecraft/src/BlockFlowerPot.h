#pragma once

#include "Block.h"

class ItemStack;

// net.minecraft.src.BlockFlowerPot
class BlockFlowerPot : public Block
{
public:
	explicit BlockFlowerPot(int_t id);

	void setBlockBoundsForItemRender() override;
	bool isOpaqueCube() override { return false; }
	int_t getRenderType() override { return 33; }
	bool renderAsNormalBlock() override { return false; }
	bool isFlowerPot() override { return true; }

	bool blockActivated(World *world, int_t i, int_t j, int_t k, EntityPlayer *entityplayer) override;
	int_t idPicked(World *world, int_t i, int_t j, int_t k);
	int_t getDamageValue(World *world, int_t i, int_t j, int_t k);
	bool canPlaceBlockAt(World *world, int_t i, int_t j, int_t k) override;
	void onNeighborBlockChange(World *world, int_t i, int_t j, int_t k, int_t l) override;
	void dropBlockAsItemWithChance(World *world, int_t i, int_t j, int_t k, int_t metadata, float chance, int_t fortune) override;
	int_t idDropped(int_t metadata, Random &random) override;

	static ItemStack *getPlantForMeta(int_t meta);
	static int_t getMetaForPlant(ItemStack *stack);
};
