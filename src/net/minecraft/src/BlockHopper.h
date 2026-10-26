#pragma once

#include "BlockContainer.h"

class TileEntityHopper;

// net.minecraft.src.BlockHopper
class BlockHopper : public BlockContainer
{
public:
	BlockHopper(int_t id);

	void setBlockBoundsBasedOnState(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k) override;
	void getCollidingBoundingBoxes(World *world, int_t i, int_t j, int_t k, AxisAlignedBB *axisalignedbb, std::vector<AxisAlignedBB *> &list) override;
	void onBlockPlaced(World *world, int_t i, int_t j, int_t k, int_t l) override;
	TileEntity *getBlockEntity() override;
	void onBlockAdded(World *world, int_t i, int_t j, int_t k) override;
	bool blockActivated(World *world, int_t i, int_t j, int_t k, EntityPlayer *entityplayer) override;
	void onNeighborBlockChange(World *world, int_t i, int_t j, int_t k, int_t l) override;
	void onBlockRemoval(World *world, int_t i, int_t j, int_t k) override;
	int_t getRenderType() override;
	bool renderAsNormalBlock() override;
	bool isOpaqueCube() override;
	bool shouldSideBeRendered(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k, int_t l) override;
	int_t getBlockTextureFromSideAndMetadata(int_t i, int_t j) override;
	int_t getBlockTextureFromSide(int_t i) override;
	bool hasComparatorInputOverride() override;
	int_t getComparatorInputOverride(World *world, int_t i, int_t j, int_t k, int_t l) override;
	const char *getItemIconName() const override;

	static int_t getDirectionFromMetadata(int_t metadata);
	static bool getIsBlockNotPoweredFromMetadata(int_t metadata);
	static int_t getHopperIcon(const std::string &name);
	static TileEntityHopper *getHopperTile(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k);

private:
	void updateMetadata(World *world, int_t i, int_t j, int_t k);
};
