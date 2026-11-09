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
#include "EntityWither.h"

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

bool BlockSkull::func_82528_d(World *world, int_t x, int_t y, int_t z, int_t skullType)
{
	if (world->getBlockId(x, y, z) != blockID)
		return false;
	TileEntitySkull *skull = dynamic_cast<TileEntitySkull *>(world->getBlockTileEntity(x, y, z));
	return skull != nullptr && skull->getSkullType() == skullType;
}

void BlockSkull::makeWither(World *world, int_t x, int_t y, int_t z, TileEntitySkull *skull)
{
	if (skull == nullptr || skull->getSkullType() != 1 || y < 2 || world->difficultySetting <= 0 || world->multiplayerWorld)
		return;

	const int_t sand = Block::slowSand->blockID;

	// Along Z: arms at (x, y-1, z+o..z+o+2), stem at (x, y-2, z+o+1).
	for (int_t o = -2; o <= 0; ++o)
	{
		if (world->getBlockId(x, y - 1, z + o) == sand && world->getBlockId(x, y - 1, z + o + 1) == sand &&
			world->getBlockId(x, y - 2, z + o + 1) == sand && world->getBlockId(x, y - 1, z + o + 2) == sand &&
			func_82528_d(world, x, y, z + o, 1) && func_82528_d(world, x, y, z + o + 1, 1) &&
			func_82528_d(world, x, y, z + o + 2, 1))
		{
			// Metadata 8 first: the skulls vanish without dropping.
			for (int_t i = 0; i < 3; ++i)
				world->setBlockMetadataWithNotify152(x, y, z + o + i, 8, 2);
			for (int_t i = 0; i < 3; ++i)
				world->setBlockNotify152(x, y, z + o + i, 0, 0, 2);
			for (int_t i = 0; i < 3; ++i)
				world->setBlockNotify152(x, y - 1, z + o + i, 0, 0, 2);
			world->setBlockNotify152(x, y - 2, z + o + 1, 0, 0, 2);

			EntityWither *wither = new EntityWither(world);
			wither->setLocationAndAngles(static_cast<double>(x) + 0.5, static_cast<double>(y) - 1.45,
				static_cast<double>(z + o) + 1.5, 90.0f, 0.0f);
			wither->renderYawOffset = 90.0f;
			wither->func_82206_m();
			world->spawnEntityInWorld(wither);

			for (int_t i = 0; i < 120; ++i)
			{
				world->spawnParticle("snowballpoof", static_cast<double>(x) + world->rand.nextDouble(),
					static_cast<double>(y - 2) + world->rand.nextDouble() * 3.9,
					static_cast<double>(z + o + 1) + world->rand.nextDouble(), 0.0, 0.0, 0.0);
			}

			for (int_t i = 0; i < 3; ++i)
			{
				world->notifyBlockChange(x, y, z + o + i, 0);
				world->notifyBlockChange(x, y - 1, z + o + i, 0);
			}
			world->notifyBlockChange(x, y - 2, z + o + 1, 0);
			return;
		}
	}

	// Along X.
	for (int_t o = -2; o <= 0; ++o)
	{
		if (world->getBlockId(x + o, y - 1, z) == sand && world->getBlockId(x + o + 1, y - 1, z) == sand &&
			world->getBlockId(x + o + 1, y - 2, z) == sand && world->getBlockId(x + o + 2, y - 1, z) == sand &&
			func_82528_d(world, x + o, y, z, 1) && func_82528_d(world, x + o + 1, y, z, 1) &&
			func_82528_d(world, x + o + 2, y, z, 1))
		{
			for (int_t i = 0; i < 3; ++i)
				world->setBlockMetadataWithNotify152(x + o + i, y, z, 8, 2);
			for (int_t i = 0; i < 3; ++i)
				world->setBlockNotify152(x + o + i, y, z, 0, 0, 2);
			for (int_t i = 0; i < 3; ++i)
				world->setBlockNotify152(x + o + i, y - 1, z, 0, 0, 2);
			world->setBlockNotify152(x + o + 1, y - 2, z, 0, 0, 2);

			EntityWither *wither = new EntityWither(world);
			wither->setLocationAndAngles(static_cast<double>(x + o) + 1.5, static_cast<double>(y) - 1.45,
				static_cast<double>(z) + 0.5, 0.0f, 0.0f);
			wither->func_82206_m();
			world->spawnEntityInWorld(wither);

			for (int_t i = 0; i < 120; ++i)
			{
				world->spawnParticle("snowballpoof", static_cast<double>(x + o + 1) + world->rand.nextDouble(),
					static_cast<double>(y - 2) + world->rand.nextDouble() * 3.9,
					static_cast<double>(z) + world->rand.nextDouble(), 0.0, 0.0, 0.0);
			}

			for (int_t i = 0; i < 3; ++i)
			{
				world->notifyBlockChange(x + o + i, y, z, 0);
				world->notifyBlockChange(x + o + i, y - 1, z, 0);
			}
			world->notifyBlockChange(x + o + 1, y - 2, z, 0);
			return;
		}
	}
}
