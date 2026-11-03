#include "EntityItem.h"
#include "DataWatcher.h"
#include "DamageSource.h"
#include "java/Math.h"
#include "java/Arithmetic.h"

#include <cmath>
#include <algorithm>
#include <typeinfo>

#include "AchievementList.h"
#include "Block.h"
#include "EntityPlayer.h"
#include "InventoryPlayer.h"
#include "Item.h"
#include "ItemStack.h"
#include "MathHelper.h"
#include "Material.h"
#include "NBTTagCompound.h"
#include "World.h"
#include "AxisAlignedBB.h"
#if PLATFORM_PS2
#include "ps2/diagnostics/Ps2OptimizationValidation.h"
#endif

EntityItem::EntityItem(World *world, double d, double d1, double d2, ItemStack *itemstack)
	: Entity(world)
{
	ensureEntityInit();
	age = 0;
	health = 5;
	hoverStart = (float)(Math::random() * 3.1415926535897931 * 2.0);
	setSize(0.25f, 0.25f);
	yOffset = height / 2.0f;
	setPosition(d, d1, d2);
	setEntityItemStack(itemstack);
	rotationYaw = (float)(Math::random() * 360.0);
	motionX = (float)(Math::random() * 0.20000000298023224 - 0.10000000149011612);
	motionY = 0.20000000298023224;
	motionZ = (float)(Math::random() * 0.20000000298023224 - 0.10000000149011612);
}

EntityItem::EntityItem(World *world)
	: Entity(world)
{
	ensureEntityInit();
	age = 0;
	health = 5;
	hoverStart = (float)(Math::random() * 3.1415926535897931 * 2.0);
	setSize(0.25f, 0.25f);
	yOffset = height / 2.0f;
}

EntityItem::~EntityItem()
{
}

bool EntityItem::canTriggerWalking()
{
	return false;
}

void EntityItem::entityInit()
{
	if (!getDataWatcher()->hasObject(10))
		getDataWatcher()->addObject(10, static_cast<ItemStack *>(nullptr));
}

void EntityItem::onUpdate()
{
	Entity::onUpdate();
	if (delayBeforeCanPickup > 0)
	{
		delayBeforeCanPickup--;
	}
	prevPosX = posX;
	prevPosY = posY;
	prevPosZ = posZ;
	const int oldX = MathHelper::floor_double(prevPosX);
	const int oldY = MathHelper::floor_double(prevPosY);
	const int oldZ = MathHelper::floor_double(prevPosZ);

#if PLATFORM_PS2
	// Multiplayer item positions are server-authoritative. Once an item has
	// settled, most client ticks only need the base environmental update plus
	// pickup/despawn bookkeeping; repeating collision resolution every tick is
	// redundant. Keep one full physics tick out of four so removed support or a
	// server correction is reflected within 0.2 seconds at 20 TPS. Water/lava
	// and any meaningful motion always stay on the full path.
	if (worldObj->multiplayerWorld && onGround && !isInWater() && fire == 0)
	{
		const double horizontalMotionSq = motionX * motionX + motionZ * motionZ;
		const bool nearlyStill = horizontalMotionSq <= 0.0001 &&
		                         motionY >= -0.03 && motionY <= 0.03;
		if (nearlyStill && (ticksExisted & 3) != 0)
		{
#ifdef PS2_OPTIMIZATION_VALIDATION
			Ps2OptimizationValidation::multiplayerItemPhysics(true);
#endif
			if (++age >= 6000)
			{
				setEntityDead();
			}
			return;
		}
	}
#endif

#if defined(PS2_OPTIMIZATION_VALIDATION)
	if (worldObj->multiplayerWorld)
		Ps2OptimizationValidation::multiplayerItemPhysics(false);
	#endif
	motionY -= 0.039999999105930328;
	pushOutOfBlocks(posX, (boundingBox->minY + boundingBox->maxY) / 2.0, posZ);
	moveEntity(motionX, motionY, motionZ);
	const bool movedBlock = oldX != MathHelper::floor_double(posX) ||
	                         oldY != MathHelper::floor_double(posY) ||
	                         oldZ != MathHelper::floor_double(posZ);
	if (movedBlock || ticksExisted % 25 == 0)
	{
		if (worldObj->getBlockMaterial(MathHelper::floor_double(posX), MathHelper::floor_double(posY), MathHelper::floor_double(posZ)) == Material::lava)
		{
			motionY = 0.20000000298023224;
			motionX = rand.nextFloatDifference() * 0.2f;
			motionZ = rand.nextFloatDifference() * 0.2f;
			worldObj->playSoundAtEntity(this, "random.fizz", 0.4f, 2.0f + rand.nextFloat() * 0.4f);
		}
		if (!worldObj->multiplayerWorld)
			searchForOtherItemsNearby();
	}
	float f = 0.98f;
	if (onGround)
	{
		f = 0.1f * 0.1f * 58.8f;
		int i = worldObj->getBlockId(MathHelper::floor_double(posX), MathHelper::floor_double(boundingBox->minY) - 1, MathHelper::floor_double(posZ));
		if (i > 0)
		{
			f = Block::blocksList[i]->slipperiness * 0.98f;
		}
	}
	motionX *= (double)f;
	motionY *= 0.98000001907348633;
	motionZ *= (double)f;
	if (onGround)
	{
		motionY *= -0.5;
	}
	age++;
	if (age >= 6000)
	{
		setEntityDead();
	}
}

bool EntityItem::handleWaterMovement()
{
	return worldObj->handleMaterialAcceleration(boundingBox, Material::water, this);
}

void EntityItem::dealFireDamage(int_t i)
{
	attackEntityFrom(DamageSource::inFire, i);
}

bool EntityItem::attackEntityFrom(Entity *entity, int_t i)
{
	setBeenAttacked();
	health -= i;
	if (health <= 0)
	{
		setEntityDead();
	}
	return false;
}

bool EntityItem::attackEntityFrom(const DamageSource &source, int_t damage)
{
	if (getEntityItem() != nullptr && Item::netherStar != nullptr &&
	    getEntityItem()->itemID == Item::netherStar->shiftedIndex && source.isExplosion())
		return false;
	return attackEntityFrom(source.getEntity(), damage);
}

void EntityItem::writeEntityToNBT(NBTTagCompound *nbttagcompound)
{
	nbttagcompound->setShort("Health", static_cast<short_t>(JavaArithmetic::byteFromBits(static_cast<ubyte_t>(health))));
	nbttagcompound->setShort("Age", (short)age);
	ItemStack *stack = getEntityItem();
	if (stack != nullptr)
		nbttagcompound->setCompoundTag("Item", stack->writeToNBT(new NBTTagCompound()));
}

void EntityItem::readEntityFromNBT(NBTTagCompound *nbttagcompound)
{
	health = nbttagcompound->getShort("Health") & 0xFF;
	age = nbttagcompound->getShort("Age");
	NBTTagCompound *nbttagcompound1 = nbttagcompound->getCompoundTag("Item");
	setEntityItemStack(ItemStack::loadItemStackFromNBT(nbttagcompound1));
	if (getEntityItem() == nullptr)
		setDead();
}

void EntityItem::onCollideWithPlayer(EntityPlayer *entityplayer)
{
	if (worldObj->multiplayerWorld)
	{
		return;
	}
	ItemStack *stack = getEntityItem();
	if (stack == nullptr)
		return;
	int i = stack->stackSize;
	if (delayBeforeCanPickup == 0 && entityplayer->inventory->addItemStackToInventory(stack))
	{
		if (stack->itemID == Block::wood->blockID)
		{
			entityplayer->triggerAchievement(AchievementList::mineWood);
		}
		if (stack->itemID == Item::leather->shiftedIndex)
		{
			entityplayer->triggerAchievement(AchievementList::killCow);
		}
		if (stack->itemID == Item::diamond->shiftedIndex)
		{
			entityplayer->triggerAchievement(AchievementList::diamonds);
		}
		if (stack->itemID == Item::blazeRod->shiftedIndex)
		{
			entityplayer->triggerAchievement(AchievementList::blazeRod);
		}
		worldObj->playSoundAtEntity(this, "random.pop", 0.2f, (rand.nextFloatDifference() * 0.7f + 1.0f) * 2.0f);
		entityplayer->onItemPickup(this, i);
		if (stack->stackSize <= 0)
		{
			setEntityDead();
		}
	}
}

ItemStack *EntityItem::getEntityItem()
{
	return getDataWatcher()->getWatchableObjectItemStack(10);
}

void EntityItem::setEntityItemStack(ItemStack *stack)
{
	getDataWatcher()->updateObject(10, stack);
	getDataWatcher()->setObjectWatched(10);
}

void EntityItem::setAgeToCreativeDespawnTime()
{
	age = 4800;
}

void EntityItem::searchForOtherItemsNearby()
{
	AxisAlignedBB *searchBox = boundingBox->expand(0.5, 0.0, 0.5);
	// expand() hands out a box from the AABB pool: it must not be deleted.
	const std::vector<Entity *> nearby = worldObj->getEntitiesWithinAABB(typeid(EntityItem), searchBox);
	for (Entity *entity : nearby)
	{
		EntityItem *other = dynamic_cast<EntityItem *>(entity);
		if (other != nullptr && combineItems(other) && !isEntityAlive())
			break;
	}
}

bool EntityItem::combineItems(EntityItem *other)
{
	if (other == nullptr || other == this || !other->isEntityAlive() || !isEntityAlive())
		return false;
	ItemStack *thisStack = getEntityItem();
	ItemStack *otherStack = other->getEntityItem();
	if (thisStack == nullptr || otherStack == nullptr || otherStack->getItem() != thisStack->getItem())
		return false;
	const bool thisHasTag = thisStack->hasTagCompound();
	const bool otherHasTag = otherStack->hasTagCompound();
	if (thisHasTag != otherHasTag || (thisHasTag && !thisStack->getTagCompound()->equals(otherStack->getTagCompound())))
		return false;
	Item *itemType = thisStack->getItem();
	if (itemType != nullptr && itemType->getHasSubtypes() && otherStack->getItemDamage() != thisStack->getItemDamage())
		return false;
	if (otherStack->stackSize < thisStack->stackSize)
		return other->combineItems(this);
	if (otherStack->stackSize + thisStack->stackSize > otherStack->getMaxStackSize())
		return false;
	otherStack->stackSize += thisStack->stackSize;
	other->delayBeforeCanPickup = std::max(other->delayBeforeCanPickup, delayBeforeCanPickup);
	other->age = std::min(other->age, age);
	other->setEntityItemStack(otherStack);
	setDead();
	return true;
}

bool EntityItem::canAttackWithItem()
{
	return false;
}
