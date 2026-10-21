#include "BlockSkull.h"

#include "AxisAlignedBB.h"
#include "EntityLiving.h"
#include "EntityPlayer.h"
#include "IBlockAccess.h"
#include "Item.h"
#include "ItemStack.h"
#include "Material.h"
#include "MathHelper.h"
#include "TileEntitySkull.h"
#include "World.h"

BlockSkull::BlockSkull(int_t id)
	: BlockContainer(id, Material::circuits)
{
	setBlockBounds(0.25f, 0.0f, 0.25f, 0.75f, 0.5f, 0.75f);
}

void BlockSkull::setBlockBoundsBasedOnState(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k)
{
	const int_t side = iblockaccess->getBlockMetadata(i, j, k) & 7;
	switch (side)
	{
	case 1:
	default:
		setBlockBounds(0.25f, 0.0f, 0.25f, 0.75f, 0.5f, 0.75f);
		break;
	case 2:
		setBlockBounds(0.25f, 0.25f, 0.5f, 0.75f, 0.75f, 1.0f);
		break;
	case 3:
		setBlockBounds(0.25f, 0.25f, 0.0f, 0.75f, 0.75f, 0.5f);
		break;
	case 4:
		setBlockBounds(0.5f, 0.25f, 0.25f, 1.0f, 0.75f, 0.75f);
		break;
	case 5:
		setBlockBounds(0.0f, 0.25f, 0.25f, 0.5f, 0.75f, 0.75f);
		break;
	}
}

AxisAlignedBB *BlockSkull::getCollisionBoundingBoxFromPool(World *world, int_t i, int_t j, int_t k)
{
	setBlockBoundsBasedOnState(world, i, j, k);
	return BlockContainer::getCollisionBoundingBoxFromPool(world, i, j, k);
}

void BlockSkull::onBlockPlacedBy(World *world, int_t i, int_t j, int_t k, EntityLiving *entityliving)
{
	const int_t rot = MathHelper::floor_double((double)(entityliving->rotationYaw * 4.0f / 360.0f) + 2.5) & 3;
	world->setBlockMetadataWithNotify(i, j, k, rot);
}

TileEntity *BlockSkull::createNewTileEntity(World *world)
{
	(void)world;
	return new TileEntitySkull();
}

int_t BlockSkull::idPicked(World *world, int_t i, int_t j, int_t k)
{
	(void)world; (void)i; (void)j; (void)k;
	return Item::skull != nullptr ? Item::skull->shiftedIndex : 397;
}

int_t BlockSkull::getDamageValue(World *world, int_t i, int_t j, int_t k)
{
	TileEntity *te = world->getBlockTileEntity(i, j, k);
	TileEntitySkull *skull = dynamic_cast<TileEntitySkull *>(te);
	if (skull != nullptr)
	{
		return skull->getSkullType();
	}
	return 0;
}

void BlockSkull::dropBlockAsItemWithChance(World *world, int_t i, int_t j, int_t k, int_t metadata, float chance, int_t fortune)
{
	(void)world; (void)i; (void)j; (void)k; (void)metadata; (void)chance; (void)fortune;
}

void BlockSkull::onBlockHarvested(World *world, int_t i, int_t j, int_t k, int_t metadata, EntityPlayer *entityplayer)
{
	if (entityplayer != nullptr && entityplayer->capabilities.isCreativeMode)
	{
		metadata |= 8;
		world->setBlockMetadataWithNotify(i, j, k, metadata);
	}
	BlockContainer::onBlockHarvested(world, i, j, k, metadata, entityplayer);
}

void BlockSkull::onBlockRemoval(World *world, int_t i, int_t j, int_t k)
{
	if (!world->multiplayerWorld)
	{
		const int_t meta = world->getBlockMetadata(i, j, k);
		if ((meta & 8) == 0)
		{
			const int_t dmg = getDamageValue(world, i, j, k);
			ItemStack *drop = new ItemStack(Item::skull != nullptr ? Item::skull->shiftedIndex : 397, 1, dmg);
			dropBlockAsItem_do(world, i, j, k, drop);
		}
	}
	BlockContainer::onBlockRemoval(world, i, j, k);
}

int_t BlockSkull::idDropped(int_t metadata, Random &random)
{
	(void)metadata;
	(void)random;
	return Item::skull != nullptr ? Item::skull->shiftedIndex : 397;
}
