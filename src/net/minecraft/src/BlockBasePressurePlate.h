#pragma once
#include "Block.h"

class AxisAlignedBB;
class Entity;
class IBlockAccess;
class Material;
class World;

// net.minecraft.src.BlockBasePressurePlate
class BlockBasePressurePlate : public Block
{
protected:
	BlockBasePressurePlate(int_t id, int_t textureIndex, Material *material);

public:
	void setBlockBoundsBasedOnState(IBlockAccess *iblockaccess, int_t x, int_t y, int_t z) override;
	int_t tickRate() override;
	AxisAlignedBB *getCollisionBoundingBoxFromPool(World *world, int_t x, int_t y, int_t z) override;
	bool isOpaqueCube() override;
	bool renderAsNormalBlock() override;
	bool getBlocksMovement(IBlockAccess *iblockaccess, int_t x, int_t y, int_t z) override;
	bool canPlaceBlockAt(World *world, int_t x, int_t y, int_t z) override;
	void onNeighborBlockChange(World *world, int_t x, int_t y, int_t z, int_t neighborId) override;
	void updateTick(World *world, int_t x, int_t y, int_t z, Random &random) override;
	void onEntityCollidedWithBlock(World *world, int_t x, int_t y, int_t z, Entity *entity) override;
	void onBlockRemoval(World *world, int_t x, int_t y, int_t z) override;

	int_t isProvidingWeakPower(IBlockAccess *iblockaccess, int_t x, int_t y, int_t z, int_t side) override;
	int_t isProvidingStrongPower(IBlockAccess *iblockaccess, int_t x, int_t y, int_t z, int_t side) override;
	bool isPoweringTo(IBlockAccess *iblockaccess, int_t x, int_t y, int_t z, int_t side) override;
	bool isIndirectlyPoweringTo(World *world, int_t x, int_t y, int_t z, int_t side) override;
	bool canProvidePower() override;

	void setBlockBoundsForItemRender() override;
	int_t getMobilityFlag() override;

protected:
	void setBoundsFromMeta(int_t meta);
	void setStateIfMobInteractsWithPlate(World *world, int_t x, int_t y, int_t z, int_t currentPower);
	AxisAlignedBB *getSensitiveAABB(int_t x, int_t y, int_t z);
	void notifyNeighbors(World *world, int_t x, int_t y, int_t z);

	virtual int_t getPlateState(World *world, int_t x, int_t y, int_t z) = 0;
	virtual int_t getPowerSupply(int_t meta) = 0;
	virtual int_t getMetaFromWeight(int_t weight) = 0;
};
