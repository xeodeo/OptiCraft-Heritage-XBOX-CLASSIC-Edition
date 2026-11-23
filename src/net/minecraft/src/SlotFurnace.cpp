#include "SlotFurnace.h"

#include "EntityPlayer.h"
#include "EntityXPOrb.h"
#include "FurnaceRecipes.h"
#include "IInventory.h"
#include "ItemStack.h"
#include "Item.h"
#include "AchievementList.h"
#include "MathHelper.h"
#include "World.h"

#include <algorithm>

namespace
{
int_t ceilingFloatInt(float value)
{
	const int_t i = static_cast<int_t>(value);
	return value > static_cast<float>(i) ? i + 1 : i;
}

int_t splitFurnaceXp(int_t xp)
{
	if (xp >= 2477) return 2477;
	if (xp >= 1237) return 1237;
	if (xp >= 617) return 617;
	if (xp >= 307) return 307;
	if (xp >= 149) return 149;
	if (xp >= 73) return 73;
	if (xp >= 37) return 37;
	if (xp >= 17) return 17;
	if (xp >= 7) return 7;
	if (xp >= 3) return 3;
	return 1;
}
} // namespace

SlotFurnace::SlotFurnace(EntityPlayer *entityplayer, IInventory *iinventory, int_t i, int_t j, int_t k) :
	Slot(iinventory, i, j, k),
	thePlayer(entityplayer)
{
}

bool SlotFurnace::isItemValid(ItemStack *itemstack)
{
	return false;
}

ItemStack *SlotFurnace::decrStackSize(int_t amount)
{
	if(getHasStack())
		removeCount += std::min(amount, getStack()->stackSize);
	return Slot::decrStackSize(amount);
}

void SlotFurnace::onCrafting(ItemStack *itemstack, int_t amount)
{
	removeCount += amount;
	onCrafting(itemstack);
}

void SlotFurnace::onCrafting(ItemStack *itemstack)
{
	itemstack->onCrafting(thePlayer->worldObj, thePlayer, removeCount);
	if (thePlayer != nullptr && thePlayer->worldObj != nullptr && !thePlayer->worldObj->multiplayerWorld)
	{
		int_t xpTotal = removeCount;
		const float xpPerItem = FurnaceRecipes::smelting()->getExperience(itemstack->itemID);
		if (xpPerItem == 0.0f)
		{
			xpTotal = 0;
		}
		else if (xpPerItem < 1.0f)
		{
			int_t base = MathHelper::floor_float(static_cast<float>(xpTotal) * xpPerItem);
			if (base < ceilingFloatInt(static_cast<float>(xpTotal) * xpPerItem) &&
			    thePlayer->worldObj->rand.nextFloat() < static_cast<float>(xpTotal) * xpPerItem - static_cast<float>(base))
			{
				++base;
			}
			xpTotal = base;
		}

		while (xpTotal > 0)
		{
			const int_t split = splitFurnaceXp(xpTotal);
			xpTotal -= split;
			EntityXPOrb *orb = new EntityXPOrb(thePlayer->worldObj, thePlayer->posX, thePlayer->posY + 0.5, thePlayer->posZ + 0.5, split);
			if (!thePlayer->worldObj->spawnEntityInWorld(orb))
				delete orb;
		}
	}
	removeCount = 0;
	if(itemstack->itemID == Item::ingotIron->shiftedIndex)
	{
		thePlayer->addStat(AchievementList::acquireIron, 1);
	}
	if(itemstack->itemID == Item::fishCooked->shiftedIndex)
	{
		thePlayer->addStat(AchievementList::cookFish, 1);
	}
}

void SlotFurnace::onPickupFromSlot(ItemStack *itemstack)
{
	onCrafting(itemstack);
	Slot::onPickupFromSlot(itemstack);
}
