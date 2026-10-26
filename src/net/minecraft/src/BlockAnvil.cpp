#include "BlockAnvil.h"
#include "Texture152Atlas.h"

#include "EntityFallingSand.h"
#include "EntityLiving.h"
#include "IBlockAccess.h"
#include "Material.h"
#include "MathHelper.h"
#include "World.h"
#include "EntityPlayerSP.h"

const char *const BlockAnvil::statuses[3] = {"intact", "slightlyDamaged", "veryDamaged"};

BlockAnvil::BlockAnvil(int_t id)
	: BlockSand(id, Material::anvil)
{
	setLightOpacity(0);
}

bool BlockAnvil::blockActivated(World *world, int_t x, int_t y, int_t z, EntityPlayer *entityplayer)
{
	if (EntityPlayerSP *player = dynamic_cast<EntityPlayerSP *>(entityplayer)) { player->displayGUIRepair(x, y, z); return true; }
	return true;
}

int_t BlockAnvil::getBlockTextureFromSideAndMetadata(int_t side, int_t metadata)
{
	if (field_82521_b == 3 && side == 1)
	{
		const int_t dmg = (metadata >> 2) % 3;
		if (dmg == 1) return TEX152_BLOCK_ICON("anvil_top_damaged_1.png"); // 1.5.2 icon: anvil_top_damaged_1.png
		if (dmg == 2) return TEX152_BLOCK_ICON("anvil_top_damaged_2.png"); // 1.5.2 icon: anvil_top_damaged_2.png
		return TEX152_BLOCK_ICON("anvil_top.png"); // 1.5.2 icon: anvil_top.png
	}
	return TEX152_BLOCK_ICON("anvil_base.png"); // 1.5.2 icon: anvil_base.png
}

void BlockAnvil::onBlockPlacedBy(World *world, int_t i, int_t j, int_t k, EntityLiving *entityliving)
{
	int_t dir = MathHelper::floor_double((double)(entityliving->rotationYaw * 4.0f / 360.0f) + 0.5) & 3;
	const int_t dmg = world->getBlockMetadata(i, j, k) >> 2;
	dir = ++dir % 4;
	if (dir == 0) world->setBlockMetadataWithNotify(i, j, k, 2 | (dmg << 2));
	if (dir == 1) world->setBlockMetadataWithNotify(i, j, k, 3 | (dmg << 2));
	if (dir == 2) world->setBlockMetadataWithNotify(i, j, k, 0 | (dmg << 2));
	if (dir == 3) world->setBlockMetadataWithNotify(i, j, k, 1 | (dmg << 2));
}

void BlockAnvil::setBlockBoundsBasedOnState(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k)
{
	const int_t dir = iblockaccess->getBlockMetadata(i, j, k) & 3;
	if (dir != 3 && dir != 1)
	{
		setBlockBounds(0.125f, 0.0f, 0.0f, 0.875f, 1.0f, 1.0f);
	}
	else
	{
		setBlockBounds(0.0f, 0.0f, 0.125f, 1.0f, 1.0f, 0.875f);
	}
}

void BlockAnvil::onStartFalling(EntityFallingSand *entity)
{
	if (entity != nullptr)
	{
		entity->setIsAnvil(true);
	}
}

void BlockAnvil::onFinishFalling(World *world, int_t i, int_t j, int_t k, int_t metadata)
{
	(void)metadata;
	world->playAuxSFX(1022, i, j, k, 0);
}
