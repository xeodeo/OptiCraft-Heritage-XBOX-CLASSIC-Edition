#pragma once
#include "Block.h"

class AxisAlignedBB;
class IBlockAccess;
class Random;
class World;

// net.minecraft.src.BlockTripWireSource
class BlockTripWireSource : public Block
{
public:
	BlockTripWireSource(int_t id);

	AxisAlignedBB *getCollisionBoundingBoxFromPool(World *world, int_t x, int_t y, int_t z) override;
	bool isOpaqueCube() override;
	bool renderAsNormalBlock() override;
	int_t getRenderType() override;
	int_t tickRate() override;
	bool canPlaceBlockOnSide(World *world, int_t x, int_t y, int_t z, int_t side) override;
	bool canPlaceBlockAt(World *world, int_t x, int_t y, int_t z) override;
	void onBlockPlaced(World *world, int_t x, int_t y, int_t z, int_t side) override;
	void onBlockAdded(World *world, int_t x, int_t y, int_t z) override;
	void onNeighborBlockChange(World *world, int_t x, int_t y, int_t z, int_t neighborId) override;
	void updateTick(World *world, int_t x, int_t y, int_t z, Random &random) override;
	void setBlockBoundsBasedOnState(IBlockAccess *iblockaccess, int_t x, int_t y, int_t z) override;
	void onBlockRemoval(World *world, int_t x, int_t y, int_t z) override;

	int_t isProvidingWeakPower(IBlockAccess *iblockaccess, int_t x, int_t y, int_t z, int_t side) override;
	int_t isProvidingStrongPower(IBlockAccess *iblockaccess, int_t x, int_t y, int_t z, int_t side) override;
	bool isPoweringTo(IBlockAccess *iblockaccess, int_t x, int_t y, int_t z, int_t side) override;
	bool isIndirectlyPoweringTo(World *world, int_t x, int_t y, int_t z, int_t side) override;
	bool canProvidePower() override;

	void func_72143_a(World *world, int_t x, int_t y, int_t z, int_t blockId, int_t meta, bool notify, int_t changedDistance, int_t changedMeta);

private:
	void playSoundEffect(World *world, int_t x, int_t y, int_t z, bool connected, bool powered, bool wasConnected, bool wasPowered);
	void notifyNeighborOfChange(World *world, int_t x, int_t y, int_t z, int_t direction);
	bool checkCanStay(World *world, int_t x, int_t y, int_t z);
};
