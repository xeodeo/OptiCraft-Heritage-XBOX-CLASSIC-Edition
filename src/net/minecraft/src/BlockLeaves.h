#pragma once

#include "BlockLeavesBase.h"

class IBlockAccess;
class EntityPlayer;
class Entity;

// net.minecraft.src.BlockLeaves
class BlockLeaves : public BlockLeavesBase
{
public:
	BlockLeaves(int_t i, int_t j);
	~BlockLeaves() override;
	int_t getBlockColor() override;
	int_t getRenderColor(int_t i) override;
	int_t colorMultiplier(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k) override;
	bool usesDefaultColorMultiplier() const override { return false; }
	void onBlockRemoval(World *world, int_t i, int_t j, int_t k) override;
	void updateTick(World *world, int_t i, int_t j, int_t k, Random &random) override;
	int_t quantityDropped(Random &random) override;
	int_t idDropped(int_t i, Random &random, int_t fortune) override;
	void dropBlockAsItemWithChance(World *world, int_t i, int_t j, int_t k, int_t l, float f, int_t fortune) override;
	void harvestBlock(World *world, EntityPlayer *entityplayer, int_t i, int_t j, int_t k, int_t l) override;
	bool isOpaqueCube() override;
	bool usesStaticOpaqueCubeLookup() const override { return false; }
	int_t getBlockTextureFromSideAndMetadata(int_t i, int_t j) override;
	void getSubBlocks(int_t id, CreativeTabs *tab, std::vector<ItemStack *> &list) override;
	void setGraphicsLevel(bool flag);
	void onEntityWalking(World *world, int_t i, int_t j, int_t k, Entity *entity) override;

protected:
	int_t damageDropped(int_t i) override;

private:
	void removeLeaves(World *world, int_t i, int_t j, int_t k);
	int_t baseIndexInPNG;
	int_t *adjacentTreeBlocks;
};
