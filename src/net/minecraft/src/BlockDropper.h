#pragma once

#include "BlockDispenser.h"

// net.minecraft.src.BlockDropper
class BlockDropper : public BlockDispenser
{
public:
	BlockDropper(int_t id);

	int_t idDropped(int_t i, Random &random) override;
	int_t getBlockTexture(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k, int_t l) override;
	int_t getBlockTextureFromSide(int_t i) override;
	void onBlockPlacedBy(World *world, int_t i, int_t j, int_t k, EntityLiving *entityliving) override;
	void onNeighborBlockChange(World *world, int_t i, int_t j, int_t k, int_t l) override;
	void updateTick(World *world, int_t i, int_t j, int_t k, Random &random) override;

protected:
	TileEntity *getBlockEntity() override;
	void dispenseItem(World *world, int_t i, int_t j, int_t k, Random &random) override;
};
