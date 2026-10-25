#include "EntitySkeleton.h"

#include <cmath>
#include "java/Math.h"

#include "EntityArrow.h"
#include "EntityAIArrowAttack.h"
#include "EntityAIAttackOnCollide.h"
#include "EntityAIFleeSun.h"
#include "EntityAIHurtByTarget.h"
#include "EntityAILookIdle.h"
#include "EntityAINearestAttackableTarget.h"
#include "EntityAIRestrictSun.h"
#include "EntityAISwimming.h"
#include "EntityAIWander.h"
#include "EntityAIWatchClosest.h"
#include "EntityPlayer.h"
#include "AchievementList.h"
#include "DamageSource.h"
#include "Enchantment.h"
#include "EnchantmentHelper.h"
#include "Item.h"
#include "ItemStack.h"
#include "MathHelper.h"
#include "NBTTagCompound.h"
#include "Potion.h"
#include "PotionEffect.h"
#include "World.h"
#include "WorldProviderHell.h"
#include "DataWatcher.h"

ItemStack *EntitySkeleton::defaultHeldItem = nullptr;

EntitySkeleton::EntitySkeleton(World *world)
	: EntityMob(world),
	  aiArrowAttack(new EntityAIArrowAttack(this, 0.25f, 20, 60, 15.0f)),
	  aiAttackOnCollide(new EntityAIAttackOnCollide(this, typeid(EntityPlayer), 0.31f, false)),
	  heldItemInstance(nullptr)
{
	texture = "/mob/skeleton.png";
	moveSpeed = 0.25f;
	attackStrength = 2;

	tasks.addTask(1, new EntityAISwimming(this));
	tasks.addTask(2, new EntityAIRestrictSun(this));
	tasks.addTask(3, new EntityAIFleeSun(this, moveSpeed));
	tasks.addTask(5, new EntityAIWander(this, moveSpeed));
	tasks.addTask(6, new EntityAIWatchClosest(this, typeid(EntityPlayer), 8.0f));
	tasks.addTask(6, new EntityAILookIdle(this));
	targetTasks.addTask(1, new EntityAIHurtByTarget(this, false));
	targetTasks.addTask(2, new EntityAINearestAttackableTarget(this, typeid(EntityPlayer), 16.0f, 0, true));

	if (defaultHeldItem == nullptr)
	{
		defaultHeldItem = new ItemStack(Item::bow, 1);
	}

	if (world != nullptr && !world->multiplayerWorld)
	{
		setCombatTask();
	}
}

EntitySkeleton::~EntitySkeleton()
{
	tasks.removeTask(aiArrowAttack);
	tasks.removeTask(aiAttackOnCollide);
	delete aiArrowAttack;
	delete aiAttackOnCollide;
	delete heldItemInstance;
	heldItemInstance = nullptr;
}

void EntitySkeleton::entityInit()
{
	EntityMob::entityInit();
	dataWatcher->addObject(13, static_cast<byte_t>(0));
}

int_t EntitySkeleton::getSkeletonType() const
{
	return dataWatcher->getWatchableObjectByte(13);
}

void EntitySkeleton::setSkeletonType(int_t type)
{
	dataWatcher->updateObject(13, static_cast<byte_t>(type));
	immuneToFire = (type == 1);
	if (type == 1)
	{
		setSize(0.72f, 2.34f);
	}
	else
	{
		setSize(0.6f, 1.8f);
	}
}

const char *EntitySkeleton::getEntityTexture()
{
	return getSkeletonType() == 1 ? "/mob/skeleton_wither.png" : "/mob/skeleton.png";
}

void EntitySkeleton::setCombatTask()
{
	tasks.removeTask(aiAttackOnCollide);
	tasks.removeTask(aiArrowAttack);
	ItemStack *held = getHeldItem();
	if (held != nullptr && Item::bow != nullptr && held->itemID == Item::bow->shiftedIndex)
	{
		tasks.addTask(4, aiArrowAttack, false);
	}
	else
	{
		tasks.addTask(4, aiAttackOnCollide, false);
	}
}

void EntitySkeleton::initCreature()
{
	if (dynamic_cast<WorldProviderHell *>(worldObj->worldProvider) != nullptr && rand.nextInt(5) > 0)
	{
		setSkeletonType(1);
		delete heldItemInstance;
		heldItemInstance = new ItemStack(Item::swordStone);
	}
	else
	{
		setSkeletonType(0);
		delete heldItemInstance;
		heldItemInstance = new ItemStack(Item::bow);
	}
	setCombatTask();
}

void EntitySkeleton::attackEntityWithRangedAttack(EntityLiving *target, float distanceFactor)
{
	EntityArrow *arrow = new EntityArrow(worldObj, this, target, 1.6f, static_cast<float>(14 - worldObj->difficultySetting * 4));
	int_t power = EnchantmentHelper::getEnchantmentLevel(Enchantment::power->effectId, getHeldItem());
	int_t punch = EnchantmentHelper::getEnchantmentLevel(Enchantment::punch->effectId, getHeldItem());
	arrow->setDamage(static_cast<double>(distanceFactor * 2.0f) + rand.nextGaussian() * 0.25 + static_cast<double>(static_cast<float>(worldObj->difficultySetting) * 0.11f));
	if (power > 0)
	{
		arrow->setDamage(arrow->getDamage() + static_cast<double>(power) * 0.5 + 0.5);
	}
	if (punch > 0)
	{
		arrow->setKnockbackStrength(punch);
	}
	if (EnchantmentHelper::getEnchantmentLevel(Enchantment::flame->effectId, getHeldItem()) > 0 || getSkeletonType() == 1)
	{
		arrow->setFire(100);
	}
	worldObj->playSoundAtEntity(this, "random.bow", 1.0f, 1.0f / (rand.nextFloat() * 0.4f + 0.8f));
	worldObj->spawnEntityInWorld(arrow);
}

bool EntitySkeleton::attackEntityAsMob(Entity *entity)
{
	if (getSkeletonType() == 1)
	{
		attackStrength = 4;
		ItemStack *held = getHeldItem();
		if (held != nullptr && held->getItem() != nullptr)
		{
			attackStrength += held->getItem()->getDamageVsEntity(this);
		}
	}
	else
	{
		attackStrength = 2;
	}

	if (EntityMob::attackEntityAsMob(entity))
	{
		if (getSkeletonType() == 1 && dynamic_cast<EntityLiving *>(entity) != nullptr && Potion::wither != nullptr)
		{
			static_cast<EntityLiving *>(entity)->addPotionEffect(new PotionEffect(Potion::wither->id, 200, 0));
		}
		return true;
	}
	return false;
}

void EntitySkeleton::onLivingUpdate()
{
	if (worldObj->isDaytime() && !worldObj->multiplayerWorld && getSkeletonType() == 0)
	{
		float f = getEntityBrightness(1.0f);
		if (f > 0.5f && worldObj->canBlockSeeTheSky(MathHelper::floor_double(posX), MathHelper::floor_double(posY), MathHelper::floor_double(posZ)) && rand.nextFloat() * 30.0f < (f - 0.4f) * 2.0f)
		{
			setFire(8);
		}
	}

	if (worldObj->multiplayerWorld && getSkeletonType() == 1)
	{
		setSize(0.72f, 2.34f);
	}

	EntityMob::onLivingUpdate();
}

void EntitySkeleton::onDeath(const DamageSource &source)
{
	EntityMob::onDeath(source);
	EntityArrow *arrow = dynamic_cast<EntityArrow *>(source.getSourceOfDamage());
	EntityPlayer *player = dynamic_cast<EntityPlayer *>(source.getEntity());
	if (arrow != nullptr && player != nullptr)
	{
		const double dx = player->posX - posX;
		const double dz = player->posZ - posZ;
		if (dx * dx + dz * dz >= 2500.0 && AchievementList::snipeSkeleton != nullptr)
			player->triggerAchievement(AchievementList::snipeSkeleton);
	}
}

bool EntitySkeleton::isAIEnabled()
{
	return true;
}

int_t EntitySkeleton::getMaxHealth() const
{
	return 20;
}

jstring EntitySkeleton::getLivingSound() { return "mob.skeleton"; }
jstring EntitySkeleton::getHurtSound()   { return "mob.skeletonhurt"; }
jstring EntitySkeleton::getDeathSound()  { return "mob.skeletonhurt"; }

void EntitySkeleton::writeEntityToNBT(NBTTagCompound *nbttagcompound)
{
	EntityMob::writeEntityToNBT(nbttagcompound);
	nbttagcompound->setByte("SkeletonType", static_cast<byte_t>(getSkeletonType()));
}

void EntitySkeleton::readEntityFromNBT(NBTTagCompound *nbttagcompound)
{
	EntityMob::readEntityFromNBT(nbttagcompound);
	if (nbttagcompound->hasKey("SkeletonType"))
	{
		setSkeletonType(nbttagcompound->getByte("SkeletonType"));
	}
	setCombatTask();
}

int_t EntitySkeleton::getDropItemId()
{
	return Item::arrow != nullptr ? Item::arrow->shiftedIndex : 0;
}

void EntitySkeleton::dropFewItems(bool, int_t lootingLevel)
{
	if (getSkeletonType() == 1)
	{
		int_t coalCount = rand.nextInt(3 + lootingLevel) - 1;
		for (int_t i = 0; i < coalCount; ++i)
		{
			if (Item::coal != nullptr)
				dropItem(Item::coal->shiftedIndex, 1);
		}
	}
	else
	{
		int_t arrowCount = rand.nextInt(3 + lootingLevel);
		for (int_t i = 0; i < arrowCount; ++i)
		{
			if (Item::arrow != nullptr)
				dropItem(Item::arrow->shiftedIndex, 1);
		}
	}

	int_t boneCount = rand.nextInt(3 + lootingLevel);
	for (int_t i = 0; i < boneCount; ++i)
	{
		if (Item::bone != nullptr)
			dropItem(Item::bone->shiftedIndex, 1);
	}
}

void EntitySkeleton::dropRareDrop(int_t lootingRoll)
{
	if (getSkeletonType() == 1)
	{
		if (Item::skull != nullptr)
		{
			entityDropItem(new ItemStack(Item::skull->shiftedIndex, 1, 1), 0.0f);
		}
	}
	else
	{
		if (Item::bow == nullptr)
			return;
		if (lootingRoll > 0)
		{
			ItemStack *stack = new ItemStack(Item::bow);
			EnchantmentHelper::addRandomEnchantment(rand, stack, 5);
			entityDropItem(stack, 0.0f);
		}
		else
		{
			dropItem(Item::bow->shiftedIndex, 1);
		}
	}
}

ItemStack *EntitySkeleton::getHeldItem()
{
	return heldItemInstance != nullptr ? heldItemInstance : defaultHeldItem;
}

EnumCreatureAttribute EntitySkeleton::getCreatureAttribute() const
{
	return EnumCreatureAttribute::UNDEAD;
}
