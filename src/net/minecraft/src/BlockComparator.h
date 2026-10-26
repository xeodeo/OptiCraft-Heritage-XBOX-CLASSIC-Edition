#pragma once

#include "BlockRedstoneLogic.h"

class TileEntityComparator;

// net.minecraft.src.BlockComparator
class BlockComparator : public BlockRedstoneLogic
{
public:
	BlockComparator(int_t id, bool isPowered);

	int_t idDropped(int_t metadata, Random &random) override;
	int_t getRenderType() override;
	int_t getBlockTextureFromSideAndMetadata(int_t i, int_t j) override;
	int_t getBlockTextureFromSide(int_t i) override;
	bool blockActivated(World *world, int_t i, int_t j, int_t k, EntityPlayer *entityplayer) override;
	void updateTick(World *world, int_t i, int_t j, int_t k, Random &random) override;
	void onBlockAdded(World *world, int_t i, int_t j, int_t k) override;
	void onBlockRemoval(World *world, int_t i, int_t j, int_t k) override;
	TileEntity *getBlockEntity() override;

	bool func_94490_c(int_t metadata) const;
	TileEntityComparator *getTileEntityComparator(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k);

protected:
	bool func_96470_c(int_t metadata) override;
	int_t func_94480_d(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k, int_t l) override;
	bool func_94478_d(World *world, int_t i, int_t j, int_t k, int_t l) override;
	int_t getInputStrength(World *world, int_t i, int_t j, int_t k, int_t l) override;
	void func_94479_f(World *world, int_t i, int_t j, int_t k, int_t l) override;
	int_t func_94481_j_(int_t metadata) override;
	BlockRedstoneLogic *func_94485_e() override;
	BlockRedstoneLogic *func_94484_i() override;

private:
	int_t func_94491_m(World *world, int_t i, int_t j, int_t k, int_t l);
	void func_96476_c(World *world, int_t i, int_t j, int_t k, Random &random);
};
