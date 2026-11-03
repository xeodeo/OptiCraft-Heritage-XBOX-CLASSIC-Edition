#pragma once

#include "BlockSand.h"

// net.minecraft.src.BlockAnvil
class BlockAnvil : public BlockSand
{
public:
	static const char *const statuses[3];

	int_t field_82521_b = 0;

	explicit BlockAnvil(int_t id);
	bool blockActivated(World *world, int_t x, int_t y, int_t z, EntityPlayer *player) override;

	bool renderAsNormalBlock() override { return false; }
	bool isOpaqueCube() override { return false; }
	int_t getBlockTextureFromSideAndMetadata(int_t side, int_t metadata) override;
	void onBlockPlacedBy(World *world, int_t i, int_t j, int_t k, EntityLiving *entityliving) override;
	int_t getRenderType() override { return 35; }
	int_t damageDropped(int_t metadata) override { return metadata >> 2; }
	void setBlockBoundsBasedOnState(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k) override;
	void onStartFalling(EntityFallingSand *entity) override;
	void onFinishFalling(World *world, int_t i, int_t j, int_t k, int_t metadata) override;
	bool shouldSideBeRendered(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k, int_t side) override { (void)iblockaccess; (void)i; (void)j; (void)k; (void)side; return true; }
	bool usesDefaultFaceCulling() const override { return false; }
	void getSubBlocks(int_t id, CreativeTabs *tab, std::vector<ItemStack *> &list) override;
};
