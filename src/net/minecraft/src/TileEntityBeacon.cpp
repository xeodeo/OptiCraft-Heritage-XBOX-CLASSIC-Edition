#include "TileEntityBeacon.h"

#include "AxisAlignedBB.h"
#include "Block.h"
#include "EntityPlayer.h"
#include "Item.h"
#include "ItemStack.h"
#include "NBTTagCompound.h"
#include "Potion.h"
#include "PotionEffect.h"
#include "World.h"

// Java's static effectsList table. The Potion pointers only exist after
// Potion::initPotions(), so they are read when asked for, not at static init.
const Potion *TileEntityBeacon::getEffect(int tier, int index)
{
	const Potion *const table[4][2] = {
		{ Potion::moveSpeed, Potion::digSpeed },
		{ Potion::resistance, Potion::jump },
		{ Potion::damageBoost, nullptr },
		{ Potion::regeneration, nullptr }
	};
	return tier >= 0 && tier < 4 && index >= 0 && index < 2 ? table[tier][index] : nullptr;
}
const int TileEntityBeacon::effectsCount[4] = { 2, 2, 1, 1 };

TileEntityBeacon::TileEntityBeacon()
{
}

TileEntityBeacon::~TileEntityBeacon()
{
	delete payment;
}

void TileEntityBeacon::updateEntity()
{
	TileEntity::updateEntity();
	if (worldObj != nullptr && worldObj->getTotalWorldTime() % 80 == 0)
	{
		updateState();
		addEffectsToPlayers();
	}
}

void TileEntityBeacon::setPrimaryEffect(int_t eff)
{
	primaryEffect = 0;
	for (int tier = 0; tier < levels && tier < 3; ++tier)
	{
		for (int i = 0; i < effectsCount[tier]; ++i)
		{
			if (getEffect(tier, i) != nullptr && getEffect(tier, i)->id == eff)
			{
				primaryEffect = eff;
				return;
			}
		}
	}
}

void TileEntityBeacon::setSecondaryEffect(int_t eff)
{
	secondaryEffect = 0;
	if (levels >= 4)
	{
		for (int tier = 0; tier < 4; ++tier)
		{
			for (int i = 0; i < effectsCount[tier]; ++i)
			{
				if (getEffect(tier, i) != nullptr && getEffect(tier, i)->id == eff)
				{
					secondaryEffect = eff;
					return;
				}
			}
		}
	}
}

void TileEntityBeacon::addEffectsToPlayers()
{
	if (isBeaconActive && levels > 0 && worldObj != nullptr && !worldObj->multiplayerWorld && primaryEffect > 0)
	{
		const double range = static_cast<double>(levels * 10 + 10);
		byte_t amp = 0;
		if (levels >= 4 && primaryEffect == secondaryEffect)
		{
			amp = 1;
		}

		AxisAlignedBB *box = AxisAlignedBB::getBoundingBoxFromPool(
			xCoord, yCoord, zCoord, xCoord + 1, yCoord + 1, zCoord + 1)->expand(range, range, range);
		box->maxY = 256.0;

		std::vector<Entity *> entities = worldObj->getEntitiesWithinAABBExcludingEntity(nullptr, box);
		for (Entity *ent : entities)
		{
			EntityPlayer *player = dynamic_cast<EntityPlayer *>(ent);
			if (player != nullptr)
			{
				player->addPotionEffect(new PotionEffect(primaryEffect, 180, amp));
				if (levels >= 4 && primaryEffect != secondaryEffect && secondaryEffect > 0)
				{
					player->addPotionEffect(new PotionEffect(secondaryEffect, 180, 0));
				}
			}
		}
	}
}

void TileEntityBeacon::updateState()
{
	if (worldObj == nullptr)
	{
		return;
	}
	if (!worldObj->canBlockSeeTheSky(xCoord, yCoord + 1, zCoord))
	{
		isBeaconActive = false;
		levels = 0;
	}
	else
	{
		isBeaconActive = true;
		levels = 0;

		for (int_t tier = 1; tier <= 4; ++tier)
		{
			const int_t layerY = yCoord - tier;
			if (layerY < 0)
			{
				break;
			}

			bool valid = true;
			for (int_t x = xCoord - tier; x <= xCoord + tier && valid; ++x)
			{
				for (int_t z = zCoord - tier; z <= zCoord + tier; ++z)
				{
					const int_t id = worldObj->getBlockId(x, layerY, z);
					if ((Block::blockEmerald == nullptr || id != Block::blockEmerald->blockID) &&
					    (Block::blockGold == nullptr || id != Block::blockGold->blockID) &&
					    (Block::blockDiamond == nullptr || id != Block::blockDiamond->blockID) &&
					    (Block::blockSteel == nullptr || id != Block::blockSteel->blockID))
					{
						valid = false;
						break;
					}
				}
			}

			if (!valid)
			{
				break;
			}
			levels = tier;
		}

		if (levels == 0)
		{
			isBeaconActive = false;
		}
	}
}

float TileEntityBeacon::func_82125_v_()
{
	if (!isBeaconActive)
	{
		return 0.0f;
	}
	if (worldObj != nullptr)
	{
		const int_t dt = static_cast<int_t>(worldObj->getTotalWorldTime() - field_82137_b);
		field_82137_b = worldObj->getTotalWorldTime();
		if (dt > 1)
		{
			field_82138_c -= static_cast<float>(dt) / 40.0f;
			if (field_82138_c < 0.0f) field_82138_c = 0.0f;
		}
		field_82138_c += 0.025f;
		if (field_82138_c > 1.0f) field_82138_c = 1.0f;
		return field_82138_c;
	}
	return 1.0f;
}

void TileEntityBeacon::readFromNBT(NBTTagCompound *nbttagcompound)
{
	TileEntity::readFromNBT(nbttagcompound);
	primaryEffect = nbttagcompound->getInteger("Primary");
	secondaryEffect = nbttagcompound->getInteger("Secondary");
	levels = nbttagcompound->getInteger("Levels");
}

void TileEntityBeacon::writeToNBT(NBTTagCompound *nbttagcompound)
{
	TileEntity::writeToNBT(nbttagcompound);
	nbttagcompound->setInteger("Primary", primaryEffect);
	nbttagcompound->setInteger("Secondary", secondaryEffect);
	nbttagcompound->setInteger("Levels", levels);
}

ItemStack *TileEntityBeacon::getStackInSlot(int_t i)
{
	return (i == 0) ? payment : nullptr;
}

ItemStack *TileEntityBeacon::decrStackSize(int_t i, int_t j)
{
	if (i == 0 && payment != nullptr)
	{
		if (payment->stackSize <= j)
		{
			ItemStack *ret = payment;
			payment = nullptr;
			return ret;
		}
		return payment->splitStack(j);
	}
	return nullptr;
}

ItemStack *TileEntityBeacon::getStackInSlotOnClosing(int_t i)
{
	if (i == 0 && payment != nullptr)
	{
		ItemStack *ret = payment;
		payment = nullptr;
		return ret;
	}
	return nullptr;
}

void TileEntityBeacon::setInventorySlotContents(int_t i, ItemStack *itemstack)
{
	if (i == 0)
	{
		delete payment;
		payment = itemstack;
	}
}

bool TileEntityBeacon::canInteractWith(EntityPlayer *entityplayer)
{
	if (worldObj == nullptr || worldObj->getBlockTileEntity(xCoord, yCoord, zCoord) != this)
	{
		return false;
	}
	return entityplayer->getDistanceSq((double)xCoord + 0.5, (double)yCoord + 0.5, (double)zCoord + 0.5) <= 64.0;
}

bool TileEntityBeacon::isStackValidForSlot(int_t slot, ItemStack *stack)
{
	if (slot != 0 || stack == nullptr)
		return false;

	return (Item::emerald != nullptr && stack->itemID == Item::emerald->itemID) ||
	       (Item::diamond != nullptr && stack->itemID == Item::diamond->itemID) ||
	       (Item::ingotGold != nullptr && stack->itemID == Item::ingotGold->itemID) ||
	       (Item::ingotIron != nullptr && stack->itemID == Item::ingotIron->itemID);
}
