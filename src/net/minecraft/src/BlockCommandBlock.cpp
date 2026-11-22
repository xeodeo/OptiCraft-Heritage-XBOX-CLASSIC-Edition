#include "BlockCommandBlock.h"

#include "EntityLiving.h"
#include "EntityPlayer.h"
#include "ItemStack.h"
#include "Material.h"
#include "Texture152Atlas.h"
#include "TileEntityCommandBlock.h"
#include "World.h"

BlockCommandBlock::BlockCommandBlock(int_t id)
	: BlockContainer(id, TEX152_BLOCK_ICON("commandBlock.png"), Material::iron)
{
}

TileEntity *BlockCommandBlock::getBlockEntity()
{
	return new TileEntityCommandBlock();
}

void BlockCommandBlock::onNeighborBlockChange(World *world, int_t x, int_t y, int_t z, int_t neighborId)
{
	(void)neighborId;
	if (world->multiplayerWorld)
		return;

	const bool powered = world->isBlockIndirectlyGettingPowered(x, y, z);
	const int_t meta = world->getBlockMetadata(x, y, z);
	const bool wasPowered = (meta & 1) != 0;
	if (powered && !wasPowered)
	{
		world->setBlockMetadataWithNotify152(x, y, z, meta | 1, 4);
		world->scheduleBlockUpdate(x, y, z, blockID, tickRate());
	}
	else if (!powered && wasPowered)
	{
		world->setBlockMetadataWithNotify152(x, y, z, meta & ~1, 4);
	}
}

void BlockCommandBlock::updateTick(World *world, int_t x, int_t y, int_t z, Random &random)
{
	(void)random;
	TileEntityCommandBlock *tile = dynamic_cast<TileEntityCommandBlock *>(world->getBlockTileEntity(x, y, z));
	if (tile != nullptr)
	{
		tile->setSignalStrength(tile->executeCommandOnPowered(world));
		world->func_96440_m(x, y, z, blockID);
	}
}

int_t BlockCommandBlock::tickRate()
{
	return 1;
}

bool BlockCommandBlock::blockActivated(World *world, int_t x, int_t y, int_t z, EntityPlayer *player)
{
	if (player == nullptr || !player->capabilities.isCreativeMode)
		return false;

	TileEntityCommandBlock *tile = dynamic_cast<TileEntityCommandBlock *>(world->getBlockTileEntity(x, y, z));
	if (tile != nullptr)
	{
		player->displayGUICommandBlock(tile);
	}
	return true;
}

bool BlockCommandBlock::hasComparatorInputOverride()
{
	return true;
}

int_t BlockCommandBlock::getComparatorInputOverride(World *world, int_t x, int_t y, int_t z, int_t side)
{
	(void)side;
	TileEntityCommandBlock *tile = dynamic_cast<TileEntityCommandBlock *>(world->getBlockTileEntity(x, y, z));
	if (tile == nullptr)
		return 0;
	int_t strength = tile->getSignalStrength();
	if (strength < 0)
		strength = 0;
	if (strength > 15)
		strength = 15;
	return strength;
}

void BlockCommandBlock::onBlockPlacedBy(World *world, int_t x, int_t y, int_t z, EntityLiving *entityliving)
{
	TileEntityCommandBlock *tile = dynamic_cast<TileEntityCommandBlock *>(world->getBlockTileEntity(x, y, z));
	if (tile != nullptr && entityliving != nullptr)
	{
		ItemStack *held = entityliving->getHeldItem();
		if (held != nullptr && held->hasDisplayName())
		{
			tile->setCommandSenderName(held->getDisplayName());
		}
	}
}

int_t BlockCommandBlock::quantityDropped(Random &random)
{
	(void)random;
	return 0;
}
