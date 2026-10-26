#pragma once

#include "BlockRedstoneLogic.h"

class EntityPlayer;

// net.minecraft.src.BlockRedstoneRepeater
class BlockRedstoneRepeater : public BlockRedstoneLogic
{
public:
	BlockRedstoneRepeater(int_t id, bool isPowered);

	int_t getBlockTextureFromSideAndMetadata(int_t i, int_t j) override;
	int_t getBlockTextureFromSide(int_t i) override;
	int_t getRenderType() override;
	bool blockActivated(World *world, int_t i, int_t j, int_t k, EntityPlayer *entityplayer) override;
	int_t idDropped(int_t metadata, Random &random) override;
	void randomDisplayTick(World *world, int_t i, int_t j, int_t k, Random &random) override;
	void onBlockRemoval(World *world, int_t i, int_t j, int_t k) override;
	bool func_94476_e(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k, int_t l) override;

	static const double isPowered[4];

protected:
	bool func_94477_d(int_t blockId) override;
	int_t func_94481_j_(int_t metadata) override;
	BlockRedstoneLogic *func_94485_e() override;
	BlockRedstoneLogic *func_94484_i() override;

private:
	static const int_t repeaterState[4];
};
