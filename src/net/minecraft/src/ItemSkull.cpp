#include "ItemSkull.h"
#include "Texture152Atlas.h"

#include "Block.h"
#include "EntityPlayer.h"
#include "ItemStack.h"
#include "Material.h"
#include "MathHelper.h"
#include "TileEntitySkull.h"
#include "World.h"

const char *const ItemSkull::skullTypes[5] = {"skeleton", "wither", "zombie", "char", "creeper"};

ItemSkull::ItemSkull(int id)
	: Item(id)
{
	setMaxDamage(0);
	setHasSubtypes(true);
}

int ItemSkull::getIconFromDamage(int damage)
{
	switch (damage)
	{
	case 0: return TEX152_ITEM_ICON("skull_skeleton.png"); // 1.5.2 icon: skull_skeleton.png
	case 1: return TEX152_ITEM_ICON("skull_wither.png");   // 1.5.2 icon: skull_wither.png
	case 2: return TEX152_ITEM_ICON("skull_zombie.png");   // 1.5.2 icon: skull_zombie.png
	case 3: return TEX152_ITEM_ICON("skull_char.png");     // 1.5.2 icon: skull_char.png
	case 4: return TEX152_ITEM_ICON("skull_creeper.png");  // 1.5.2 icon: skull_creeper.png
	default: return TEX152_ITEM_ICON("skull_skeleton.png");
	}
}

int ItemSkull::getMetadata(int damage)
{
	return damage;
}

std::string ItemSkull::getItemNameIS(ItemStack *itemstack)
{
	int damage = itemstack->getItemDamage();
	if (damage < 0 || damage >= 5)
	{
		damage = 0;
	}
	return getItemName() + "." + skullTypes[damage];
}

bool ItemSkull::onItemUse(ItemStack *itemstack, EntityPlayer *entityplayer, World *world, int i, int j, int k, int side)
{
	if (side == 0)
	{
		return false;
	}
	if (!world->getBlockMaterial(i, j, k)->isSolid())
	{
		return false;
	}

	if (side == 1) ++j;
	if (side == 2) --k;
	if (side == 3) ++k;
	if (side == 4) --i;
	if (side == 5) ++i;

	if (entityplayer != nullptr && !entityplayer->canPlayerEdit(i, j, k))
	{
		return false;
	}
	if (Block::skull == nullptr || !Block::skull->canPlaceBlockAt(world, i, j, k))
	{
		return false;
	}

	world->setBlockAndMetadataWithNotify(i, j, k, Block::skull->blockID, side);
	int rot = 0;
	if (side == 1 && entityplayer != nullptr)
	{
		rot = MathHelper::floor_double((double)(entityplayer->rotationYaw * 16.0f / 360.0f) + 0.5) & 15;
	}

	TileEntity *te = world->getBlockTileEntity(i, j, k);
	TileEntitySkull *skull = dynamic_cast<TileEntitySkull *>(te);
	if (skull != nullptr)
	{
		skull->setSkullType(itemstack->getItemDamage());
		skull->setSkullRotation(rot);
	}

	--itemstack->stackSize;
	return true;
}
