#pragma once

#include "BlockDirectional.h"

// net.minecraft.src.BlockRedstoneLogic
class BlockRedstoneLogic : public BlockDirectional
{
protected:
	bool isRepeaterPowered;

	BlockRedstoneLogic(int_t id, bool isPowered);

public:
	bool renderAsNormalBlock() override;
	bool canPlaceBlockAt(World *world, int_t i, int_t j, int_t k) override;
	bool canBlockStay(World *world, int_t i, int_t j, int_t k) override;
	void updateTick(World *world, int_t i, int_t j, int_t k, Random &random) override;
	bool shouldSideBeRendered(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k, int_t l) override;
	bool usesDefaultFaceCulling() const override { return false; }
	int_t getRenderType() override;
	int_t isProvidingStrongPower(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k, int_t l) override;
	int_t isProvidingWeakPower(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k, int_t l) override;
	bool isPoweringTo(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k, int_t l) override;
	bool isIndirectlyPoweringTo(World *world, int_t i, int_t j, int_t k, int_t l) override;
	void onNeighborBlockChange(World *world, int_t i, int_t j, int_t k, int_t l) override;
	bool canProvidePower() override;
	void onBlockPlacedBy(World *world, int_t i, int_t j, int_t k, EntityLiving *entityliving) override;
	void onBlockAdded(World *world, int_t i, int_t j, int_t k) override;
	void onBlockDestroyedByPlayer(World *world, int_t i, int_t j, int_t k, int_t metadata) override;
	bool isOpaqueCube() override;
	bool isAssociatedBlockID(int_t otherBlockId) override;

	virtual bool func_94476_e(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k, int_t l);
	static bool isRedstoneRepeaterBlockID(int_t blockId);
	bool func_94487_f(int_t blockId);
	bool func_83011_d(World *world, int_t i, int_t j, int_t k, int_t l);

protected:
	virtual bool func_96470_c(int_t metadata);
	virtual void func_94479_f(World *world, int_t i, int_t j, int_t k, int_t l);
	virtual bool func_94478_d(World *world, int_t i, int_t j, int_t k, int_t l);
	virtual int_t getInputStrength(World *world, int_t i, int_t j, int_t k, int_t l);
	int_t func_94482_f(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k, int_t l);
	int_t func_94488_g(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k, int_t l);
	virtual void func_94483_i_(World *world, int_t i, int_t j, int_t k);
	virtual bool func_94477_d(int_t blockId);
	virtual int_t func_94480_d(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k, int_t l);
	virtual int_t func_94486_g(int_t metadata);
	virtual int_t func_94481_j_(int_t metadata) = 0;
	virtual BlockRedstoneLogic *func_94485_e() = 0;
	virtual BlockRedstoneLogic *func_94484_i() = 0;
};
