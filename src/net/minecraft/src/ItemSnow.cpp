#include "ItemSnow.h"

#include "Block.h"
#include "EntityPlayer.h"
#include "ItemStack.h"
#include "StepSound.h"
#include "World.h"
#include "mods/ModManager.h"

ItemSnow::ItemSnow(int i, Block *block)
	: ItemBlock(i)
{
	(void)block;
	setMaxDamage(0);
	setHasSubtypes(true);
}

int ItemSnow::getMetadata(int damage)
{
	return damage;
}

bool ItemSnow::onItemUse(ItemStack *itemstack, EntityPlayer *entityplayer, World *world, int i, int j, int k, int l)
{
	if (!ModManager::isBlockAllowed(getBlockID()))
	{
		return false;
	}
	if (itemstack->stackSize == 0)
	{
		return false;
	}
	if (entityplayer != nullptr && !entityplayer->canPlayerEdit(i, j, k))
	{
		return false;
	}
	int targetId = world->getBlockId(i, j, k);
	if (Block::snow != nullptr && targetId == Block::snow->blockID)
	{
		Block *snowBlock = Block::snow;
		int meta = world->getBlockMetadata(i, j, k);
		int depth = meta & 7;
		if (depth <= 6 && world->checkIfAABBIsClear(snowBlock->getCollisionBoundingBoxFromPool(world, i, j, k)) &&
		    world->setBlockMetadataWithNotify(i, j, k, depth + 1, 2))
		{
			world->playSoundEffect((float)i + 0.5f, (float)j + 0.5f, (float)k + 0.5f,
			                       snowBlock->stepSound->getStepSound(),
			                       (snowBlock->stepSound->getVolume() + 1.0f) / 2.0f,
			                       snowBlock->stepSound->getPitch() * 0.8f);
			itemstack->stackSize--;
			return true;
		}
	}
	return ItemBlock::onItemUse(itemstack, entityplayer, world, i, j, k, l);
}
