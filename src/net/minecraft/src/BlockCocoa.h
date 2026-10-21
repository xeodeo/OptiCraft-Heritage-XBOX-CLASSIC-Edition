#pragma once

#include "BlockDirectional.h"

// net.minecraft.src.BlockCocoa
class BlockCocoa : public BlockDirectional
{
public:
	explicit BlockCocoa(int_t id);

	int_t getBlockTextureFromSideAndMetadata(int_t side, int_t metadata) override;
	int_t getBlockTextureFromSide(int_t side) override;
	int_t getRenderType() override { return 28; }
	bool renderAsNormalBlock() override { return false; }
	bool isOpaqueCube() override { return false; }

	void updateTick(World *world, int_t i, int_t j, int_t k, Random &random) override;
	bool canBlockStay(World *world, int_t i, int_t j, int_t k) override;
	void onNeighborBlockChange(World *world, int_t i, int_t j, int_t k, int_t l) override;
	void setBlockBoundsBasedOnState(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k) override;
	AxisAlignedBB *getCollisionBoundingBoxFromPool(World *world, int_t i, int_t j, int_t k) override;
	AxisAlignedBB *getSelectedBoundingBoxFromPool(World *world, int_t i, int_t j, int_t k) override;
	void onBlockPlacedBy(World *world, int_t i, int_t j, int_t k, EntityLiving *entityliving) override;
	int_t onBlockPlaced(World *world, int_t i, int_t j, int_t k, int_t side, float hitX, float hitY, float hitZ, int_t metadata);
	void onBlockPlaced(World *world, int_t i, int_t j, int_t k, int_t side) override;
	void dropBlockAsItemWithChance(World *world, int_t i, int_t j, int_t k, int_t metadata, float chance, int_t fortune) override;
	int_t idPicked(World *world, int_t i, int_t j, int_t k);
	int_t getDamageValue(World *world, int_t i, int_t j, int_t k);

	static int_t func_72219_c(int_t meta) { return (meta & 12) >> 2; }
	static int_t getCocoaIconForStage(int_t stage);
};
