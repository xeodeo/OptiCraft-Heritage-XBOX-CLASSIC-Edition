#pragma once

#include "Block.h"
#include <string>

class IBlockAccess;

// net.minecraft.src.BlockHalfSlab
class BlockHalfSlab : public Block
{
public:
	BlockHalfSlab(int_t id, bool isDouble, Material *material);

	void setBlockBoundsBasedOnState(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k) override;
	void setBlockBoundsForItemRender() override;
	void getCollidingBoundingBoxes(World *world, int_t i, int_t j, int_t k, AxisAlignedBB *axisalignedbb, std::vector<AxisAlignedBB *> &arraylist) override;
	bool isOpaqueCube() override;
	void onBlockPlaced(World *world, int_t i, int_t j, int_t k, int_t side) override;
	int_t onBlockPlaced(World *world, int_t i, int_t j, int_t k, int_t side, float hitX, float hitY, float hitZ, int_t metadata);
	int_t quantityDropped(Random &random) override;
	int_t damageDropped(int_t i) override;
	bool renderAsNormalBlock() override;
	bool shouldSideBeRendered(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k, int_t side) override;
	bool usesDefaultFaceCulling() const override { return false; }

	virtual std::string getFullSlabName(int_t metadata) = 0;

	static bool isBlockSingleSlab(int_t id);

protected:
	bool isDoubleSlab;
};
