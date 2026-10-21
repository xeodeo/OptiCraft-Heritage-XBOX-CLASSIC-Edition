#pragma once

#include "Block.h"

class EntityPlayer;

// net.minecraft.src.BlockButton
class BlockButton : public Block
{
public:
	BlockButton(int_t id, bool sensible);

	AxisAlignedBB *getCollisionBoundingBoxFromPool(World *world, int_t i, int_t j, int_t k) override;
	int_t tickRate() override;
	bool isOpaqueCube() override;
	bool renderAsNormalBlock() override;
	bool canPlaceBlockOnSide(World *world, int_t i, int_t j, int_t k, int_t l) override;
	bool canPlaceBlockAt(World *world, int_t i, int_t j, int_t k) override;
	void onBlockPlaced(World *world, int_t i, int_t j, int_t k, int_t l) override;
	void onNeighborBlockChange(World *world, int_t i, int_t j, int_t k, int_t l) override;
	void setBlockBoundsBasedOnState(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k) override;
	void onBlockClicked(World *world, int_t i, int_t j, int_t k, EntityPlayer *entityplayer) override;
	bool blockActivated(World *world, int_t i, int_t j, int_t k, EntityPlayer *entityplayer) override;
	void onBlockRemoval(World *world, int_t i, int_t j, int_t k) override;

	bool isPoweringTo(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k, int_t l) override;
	bool isIndirectlyPoweringTo(World *world, int_t i, int_t j, int_t k, int_t l) override;
	int_t isProvidingWeakPower(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k, int_t l) override;
	int_t isProvidingStrongPower(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k, int_t l) override;
	bool canProvidePower() override;

	void updateTick(World *world, int_t i, int_t j, int_t k, Random &random) override;
	void setBlockBoundsForItemRender() override;
	void onEntityCollidedWithBlock(World *world, int_t i, int_t j, int_t k, Entity *entity) override;

protected:
	bool sensible;

private:
	int_t getOrientation(World *world, int_t i, int_t j, int_t k);
	bool canPlaceButtonAt(World *world, int_t i, int_t j, int_t k);
	void setButtonBounds(int_t metadata);
	void notifyNeighbors(World *world, int_t i, int_t j, int_t k, int_t side);
	void checkArrowCollision(World *world, int_t i, int_t j, int_t k);
};
