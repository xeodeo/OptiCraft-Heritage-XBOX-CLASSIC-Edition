#include "EntityAgeable.h"

#include "DataWatcher.h"
#include "EntityList.h"
#include "EntityPlayer.h"
#include "InventoryPlayer.h"
#include "Item.h"
#include "ItemStack.h"
#include "NBTTagCompound.h"
#include "PlayerCapabilities.h"
#include "World.h"

EntityAgeable::EntityAgeable(World *world)
	: EntityCreature(world)
{
}

bool EntityAgeable::interact(EntityPlayer *player)
{
	if (player == nullptr || player->inventory == nullptr)
		return false;

	ItemStack *stack = player->inventory->getCurrentItem();
	if (stack != nullptr && Item::monsterPlacer != nullptr && stack->itemID == Item::monsterPlacer->shiftedIndex && !worldObj->multiplayerWorld)
	{
		const int_t entityId = EntityList::getEntityID(this);
		if (entityId > 0 && entityId == stack->getItemDamage())
		{
			EntityAgeable *child = createChild(this);
			if (child != nullptr)
			{
				child->setGrowingAge(-24000);
				child->setLocationAndAngles(posX, posY, posZ, 0.0f, 0.0f);
				if (!worldObj->spawnEntityInWorld(child))
				{
					delete child;
				}
				else if (!player->capabilities.isCreativeMode)
				{
					--stack->stackSize;
					if (stack->stackSize <= 0)
						player->inventory->setInventorySlotContents(player->inventory->currentItem, nullptr);
				}
			}
		}
	}

	return EntityCreature::interact(player);
}

void EntityAgeable::entityInit()
{
	EntityLiving::entityInit();
	dataWatcher->addObject(12, int_t(0));
}

int_t EntityAgeable::getGrowingAge()
{
	return dataWatcher->getWatchableObjectInt(12);
}

void EntityAgeable::setGrowingAge(int_t age)
{
	dataWatcher->updateObject(12, age);
	setScaleForAge(isChild());
}

void EntityAgeable::writeEntityToNBT(NBTTagCompound *tag)
{
	EntityCreature::writeEntityToNBT(tag);
	tag->setInteger("Age", getGrowingAge());
}

void EntityAgeable::readEntityFromNBT(NBTTagCompound *tag)
{
	EntityCreature::readEntityFromNBT(tag);
	setGrowingAge(tag->getInteger("Age"));
}

void EntityAgeable::onLivingUpdate()
{
	EntityCreature::onLivingUpdate();
	if (worldObj != nullptr && worldObj->multiplayerWorld)
	{
		setScaleForAge(isChild());
	}
	else
	{
		int_t age = getGrowingAge();
		if (age < 0)
			setGrowingAge(age + 1);
		else if (age > 0)
			setGrowingAge(age - 1);
	}
}

bool EntityAgeable::isChild()
{
	return getGrowingAge() < 0;
}

void EntityAgeable::setScaleForAge(bool child)
{
	setScale(child ? 0.5f : 1.0f);
}

void EntityAgeable::setSize(float width, float height)
{
	const bool hadBaseSize = field_98056_d > 0.0f;
	field_98056_d = width;
	field_98057_e = height;
	if (!hadBaseSize)
		setScale(1.0f);
}

void EntityAgeable::setScale(float scale)
{
	EntityCreature::setSize(field_98056_d * scale, field_98057_e * scale);
}
