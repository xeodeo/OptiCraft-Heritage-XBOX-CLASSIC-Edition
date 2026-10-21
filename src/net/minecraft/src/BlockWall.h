#pragma once

#include "Block.h"

// net.minecraft.src.BlockWall
class BlockWall : public Block
{
public:
	static const char *const types[2];

	BlockWall(int_t id, Block *modelBlock);

	int_t getBlockTextureFromSideAndMetadata(int_t side, int_t metadata) override;
	int_t getRenderType() override { return 32; }
	bool renderAsNormalBlock() override { return false; }
	bool isOpaqueCube() override { return false; }
	bool getBlocksMovement(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k) override { (void)iblockaccess; (void)i; (void)j; (void)k; return false; }
	void setBlockBoundsBasedOnState(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k) override;
	AxisAlignedBB *getCollisionBoundingBoxFromPool(World *world, int_t i, int_t j, int_t k) override;
	bool canConnectWallTo(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k);
	int_t damageDropped(int_t metadata) override { return metadata; }
	bool shouldSideBeRendered(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k, int_t side) override;
	bool usesDefaultFaceCulling() const override { return false; }
};
