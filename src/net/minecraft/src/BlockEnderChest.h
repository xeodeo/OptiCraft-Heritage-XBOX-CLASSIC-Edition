#pragma once

#include "BlockContainer.h"

// net.minecraft.src.BlockEnderChest
class BlockEnderChest : public BlockContainer
{
public:
	explicit BlockEnderChest(int_t id);

	bool isOpaqueCube() override { return false; }
	bool renderAsNormalBlock() override { return false; }
	int_t getRenderType() override { return 22; }
	int_t idDropped(int_t metadata, Random &random) override;
	int_t quantityDropped(Random &random) override { (void)random; return 8; }
	bool func_50074_q() override { return true; }
	void onBlockPlacedBy(World *world, int_t i, int_t j, int_t k, EntityLiving *entityliving) override;
	bool blockActivated(World *world, int_t i, int_t j, int_t k, EntityPlayer *entityplayer) override;
	TileEntity *createNewTileEntity(World *world);
	TileEntity *getBlockEntity() override { return createNewTileEntity(nullptr); }
};
