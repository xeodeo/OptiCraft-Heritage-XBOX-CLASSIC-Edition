#pragma once

#include "BlockContainer.h"

class EntityLiving;
class EntityPlayer;
class Random;
class TileEntity;
class World;

// net.minecraft.src.BlockCommandBlock (Java 1.5.2, ID 137)
class BlockCommandBlock : public BlockContainer
{
public:
	explicit BlockCommandBlock(int_t id);

	TileEntity *getBlockEntity() override;
	void onNeighborBlockChange(World *world, int_t x, int_t y, int_t z, int_t neighborId) override;
	void updateTick(World *world, int_t x, int_t y, int_t z, Random &random) override;
	int_t tickRate() override;
	bool blockActivated(World *world, int_t x, int_t y, int_t z, EntityPlayer *player) override;
	bool hasComparatorInputOverride() override;
	int_t getComparatorInputOverride(World *world, int_t x, int_t y, int_t z, int_t side) override;
	void onBlockPlacedBy(World *world, int_t x, int_t y, int_t z, EntityLiving *entityliving) override;
	int_t quantityDropped(Random &random) override;
};
