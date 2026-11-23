#pragma once

#include "Block.h"

class EntityPlayer;

// net.minecraft.src.BlockLog
class BlockLog : public Block
{
public:
	BlockLog(int_t i);
	int_t getRenderType() override { return 31; }
	int_t quantityDropped(Random &random) override;
	int_t idDropped(int_t i, Random &random) override;
	void harvestBlock(World *world, EntityPlayer *entityplayer, int_t i, int_t j, int_t k, int_t l) override;
	void onBlockRemoval(World *world, int_t i, int_t j, int_t k) override;
	void onBlockPlaced(World *world, int_t i, int_t j, int_t k, int_t side) override;
	int_t onBlockPlaced(World *world, int_t i, int_t j, int_t k, int_t side, float hitX, float hitY, float hitZ, int_t metadata);
	int_t getBlockTextureFromSideAndMetadata(int_t i, int_t j) override;
	static int_t limitToValidMetadata(int_t metadata) { return metadata & 3; }
	void getSubBlocks(int_t id, CreativeTabs *tab, std::vector<ItemStack *> &list) override;

protected:
	int_t damageDropped(int_t i) override;
	ItemStack *createStackedBlock(int_t metadata) override;
};
