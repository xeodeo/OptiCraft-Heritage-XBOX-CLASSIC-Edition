#pragma once
#include "Block.h"

class AxisAlignedBB;
class Entity;
class EntityPlayer;
class IBlockAccess;
class Random;
class World;

// net.minecraft.src.BlockTripWire
class BlockTripWire : public Block
{
public:
	BlockTripWire(int_t id);

	int_t tickRate() override;
	AxisAlignedBB *getCollisionBoundingBoxFromPool(World *world, int_t x, int_t y, int_t z) override;
	bool isOpaqueCube() override;
	bool renderAsNormalBlock() override;
	int_t getRenderBlockPass() override;
	int_t getRenderType() override;
	int_t idDropped(int_t meta, Random &random, int_t fortune) override;
	void onNeighborBlockChange(World *world, int_t x, int_t y, int_t z, int_t neighborId) override;
	void setBlockBoundsBasedOnState(IBlockAccess *iblockaccess, int_t x, int_t y, int_t z) override;
	void onBlockAdded(World *world, int_t x, int_t y, int_t z) override;
	void onBlockRemoval(World *world, int_t x, int_t y, int_t z) override;
	void harvestBlock(World *world, EntityPlayer *player, int_t x, int_t y, int_t z, int_t meta) override;
	void onEntityCollidedWithBlock(World *world, int_t x, int_t y, int_t z, Entity *entity) override;
	void updateTick(World *world, int_t x, int_t y, int_t z, Random &random) override;

	static bool isConnected(IBlockAccess *access, int_t x, int_t y, int_t z, int_t meta, int_t direction);

private:
	void notifyTripWireSources(World *world, int_t x, int_t y, int_t z, int_t meta);
	void updateTripWireState(World *world, int_t x, int_t y, int_t z);
};
