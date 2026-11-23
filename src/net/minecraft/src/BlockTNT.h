#pragma once

#include "Block.h"

class EntityTNTPrimed;
class EntityLiving;
class ItemStack;
class EntityPlayer;

// net.minecraft.src.BlockTNT
class BlockTNT : public Block
{
public:
	BlockTNT(int_t i, int_t j);
	int_t getBlockTextureFromSide(int_t i) override;
	void onBlockAdded(World *world, int_t i, int_t j, int_t k) override;
	void onNeighborBlockChange(World *world, int_t i, int_t j, int_t k, int_t l) override;
	int_t quantityDropped(Random &random) override;
	void onBlockDestroyedByExplosion(World *world, int_t i, int_t j, int_t k) override;
	void onBlockDestroyedByPlayer(World *world, int_t i, int_t j, int_t k, int_t l) override;
	void primeTnt(World *world, int_t x, int_t y, int_t z, int_t meta, EntityLiving *igniter);
	void onBlockClicked(World *world, int_t i, int_t j, int_t k, EntityPlayer *entityplayer) override;
	bool blockActivated(World *world, int_t i, int_t j, int_t k, EntityPlayer *entityplayer) override;
	void onEntityCollidedWithBlock(World *world, int_t x, int_t y, int_t z, Entity *entity) override;
	bool canDropFromExplosion(Explosion *explosion) override { (void)explosion; return false; }
	ItemStack *createStackedBlock(int_t metadata) override;
};
