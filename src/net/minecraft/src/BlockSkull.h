#pragma once

#include "BlockContainer.h"

class TileEntitySkull;

// net.minecraft.src.BlockSkull
class BlockSkull : public BlockContainer
{
public:
	explicit BlockSkull(int_t id);

	int_t getRenderType() override { return -1; }
	bool isOpaqueCube() override { return false; }
	bool renderAsNormalBlock() override { return false; }

	void setBlockBoundsBasedOnState(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k) override;
	AxisAlignedBB *getCollisionBoundingBoxFromPool(World *world, int_t i, int_t j, int_t k) override;
	void onBlockPlacedBy(World *world, int_t i, int_t j, int_t k, EntityLiving *entityliving) override;
	TileEntity *createNewTileEntity(World *world);
	TileEntity *getBlockEntity() override { return createNewTileEntity(nullptr); }
	int_t idPicked(World *world, int_t i, int_t j, int_t k);
	int_t getDamageValue(World *world, int_t i, int_t j, int_t k);
	int_t damageDropped(int_t metadata) override { return metadata; }
	void dropBlockAsItemWithChance(World *world, int_t i, int_t j, int_t k, int_t metadata, float chance, int_t fortune) override;
	void onBlockHarvested(World *world, int_t i, int_t j, int_t k, int_t metadata, EntityPlayer *entityplayer) override;
	void onBlockRemoval(World *world, int_t i, int_t j, int_t k) override;
	int_t idDropped(int_t metadata, Random &random) override;
	// Java 1.5.2: three wither skeleton skulls on a T of soul sand summon the Wither.
	void makeWither(World *world, int_t x, int_t y, int_t z, TileEntitySkull *skull);

private:
	bool func_82528_d(World *world, int_t x, int_t y, int_t z, int_t skullType);
};
