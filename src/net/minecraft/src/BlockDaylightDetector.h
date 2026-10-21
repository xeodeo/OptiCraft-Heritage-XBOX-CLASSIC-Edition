#pragma once
#include "BlockContainer.h"

// net.minecraft.src.BlockDaylightDetector
class BlockDaylightDetector : public BlockContainer
{
public:
	BlockDaylightDetector(int_t id);

	void setBlockBoundsBasedOnState(IBlockAccess *iblockaccess, int_t x, int_t y, int_t z) override;
	int_t isProvidingWeakPower(IBlockAccess *iblockaccess, int_t x, int_t y, int_t z, int_t side) override;
	int_t isProvidingStrongPower(IBlockAccess *iblockaccess, int_t x, int_t y, int_t z, int_t side) override;
	bool isPoweringTo(IBlockAccess *iblockaccess, int_t x, int_t y, int_t z, int_t side) override;
	bool isIndirectlyPoweringTo(World *world, int_t x, int_t y, int_t z, int_t side) override;
	bool canProvidePower() override;
	bool renderAsNormalBlock() override;
	bool isOpaqueCube() override;
	int_t getRenderType() override;
	int_t getBlockTextureFromSideAndMetadata(int_t side, int_t meta) override;
	TileEntity *getBlockEntity() override;
	TileEntity *createNewTileEntity(World *world);

	void updateLightLevel(World *world, int_t x, int_t y, int_t z);

private:
	int_t topTextureIndex;
	int_t sideTextureIndex;
};
