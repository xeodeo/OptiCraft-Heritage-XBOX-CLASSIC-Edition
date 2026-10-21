#include "ItemSeedFood.h"

#include "EntityPlayer.h"
#include "ItemStack.h"
#include "World.h"

ItemSeedFood::ItemSeedFood(int id, int healAmount, float saturationModifier, int cropId, int soilId) :
	ItemFood(id, healAmount, saturationModifier, false),
	cropId(cropId),
	soilId(soilId)
{
}

bool ItemSeedFood::onItemUse(ItemStack *itemstack, EntityPlayer *entityplayer, World *world, int i, int j, int k, int l)
{
	if (l != 1)
	{
		return false;
	}
	if (!entityplayer->canPlayerEdit(i, j, k) || !entityplayer->canPlayerEdit(i, j + 1, k))
	{
		return false;
	}
	int i1 = world->getBlockId(i, j, k);
	if (i1 == soilId && world->isAirBlock(i, j + 1, k))
	{
		world->setBlockWithNotify(i, j + 1, k, cropId);
		--itemstack->stackSize;
		return true;
	}
	return false;
}
