#include "BlockEnderChest.h"

#include "Block.h"
#include "EntityLiving.h"
#include "EntityPlayer.h"
#include "InventoryEnderChest.h"
#include "Material.h"
#include "MathHelper.h"
#include "TileEntityEnderChest.h"
#include "World.h"

BlockEnderChest::BlockEnderChest(int_t id)
	: BlockContainer(id, Material::rock)
{
	setBlockBounds(0.0625f, 0.0f, 0.0625f, 0.9375f, 0.875f, 0.9375f);
}

int_t BlockEnderChest::idDropped(int_t metadata, Random &random)
{
	(void)metadata;
	(void)random;
	return Block::obsidian != nullptr ? Block::obsidian->blockID : 49;
}

void BlockEnderChest::onBlockPlacedBy(World *world, int_t i, int_t j, int_t k, EntityLiving *entityliving)
{
	byte_t side = 0;
	const int_t rot = MathHelper::floor_double((double)(entityliving->rotationYaw * 4.0f / 360.0f) + 0.5) & 3;
	if (rot == 0) side = 2;
	if (rot == 1) side = 5;
	if (rot == 2) side = 3;
	if (rot == 3) side = 4;
	world->setBlockMetadataWithNotify(i, j, k, side);
}

bool BlockEnderChest::blockActivated(World *world, int_t i, int_t j, int_t k, EntityPlayer *entityplayer)
{
	if (entityplayer == nullptr)
	{
		return true;
	}
	InventoryEnderChest *enderInv = entityplayer->getInventoryEnderChest();
	TileEntity *te = world->getBlockTileEntity(i, j, k);
	TileEntityEnderChest *chest = dynamic_cast<TileEntityEnderChest *>(te);
	if (enderInv != nullptr && chest != nullptr)
	{
		if (world->isBlockNormalCube(i, j + 1, k))
		{
			return true;
		}
		if (world->multiplayerWorld)
		{
			return true;
		}
		enderInv->setAssociatedChest(chest);
		entityplayer->displayGUIChest(enderInv);
		return true;
	}
	return true;
}

TileEntity *BlockEnderChest::createNewTileEntity(World *world)
{
	(void)world;
	return new TileEntityEnderChest();
}
