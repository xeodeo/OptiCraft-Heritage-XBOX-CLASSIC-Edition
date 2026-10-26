#include "EntityZombie.h"

#include <algorithm>
#include "Item.h"
#include "ItemStack.h"
#include "EntityAIAttackOnCollide.h"
#include "EntityAIBreakDoor.h"
#include "EntityAIHurtByTarget.h"
#include "EntityAILookIdle.h"
#include "EntityAIMoveThroughVillage.h"
#include "EntityAIMoveTwardsRestriction.h"
#include "EntityAINearestAttackableTarget.h"
#include "EntityAISwimming.h"
#include "EntityAIWander.h"
#include "EntityAIWatchClosest.h"
#include "EntityPlayer.h"
#include "InventoryPlayer.h"
#include "EntityVillager.h"
#include "EntityItem.h"
#include "PathNavigate.h"
#include "MathHelper.h"
#include "World.h"
#include "Potion.h"
#include "PotionEffect.h"
#include "DataWatcher.h"
#include "NBTTagCompound.h"
#include "Block.h"
#include "DamageSource.h"

EntityZombie::EntityZombie(World *world)
	: EntityMob(world)
{
	ensureEntityInit();
	texture = "/mob/zombie.png";
	moveSpeed = 0.23f;
	attackStrength = 4;
	conversionTime = 0;
	heldItemInstance = nullptr;
	canPickUpLoot = false;

	getNavigator()->setBreakDoors(true);
	tasks.addTask(0, new EntityAISwimming(this));
	tasks.addTask(1, new EntityAIBreakDoor(this));
	tasks.addTask(2, new EntityAIAttackOnCollide(this, typeid(EntityPlayer), moveSpeed, false));
	tasks.addTask(3, new EntityAIAttackOnCollide(this, typeid(EntityVillager), moveSpeed, true));
	tasks.addTask(4, new EntityAIMoveTwardsRestriction(this, moveSpeed));
	tasks.addTask(5, new EntityAIMoveThroughVillage(this, moveSpeed, false));
	tasks.addTask(6, new EntityAIWander(this, moveSpeed));
	tasks.addTask(7, new EntityAIWatchClosest(this, typeid(EntityPlayer), 8.0f));
	tasks.addTask(7, new EntityAILookIdle(this));
	targetTasks.addTask(1, new EntityAIHurtByTarget(this, false));
	targetTasks.addTask(2, new EntityAINearestAttackableTarget(this, typeid(EntityPlayer), 16.0f, 0, true));
	targetTasks.addTask(2, new EntityAINearestAttackableTarget(this, typeid(EntityVillager), 16.0f, 0, false));
}

EntityZombie::~EntityZombie()
{
	delete heldItemInstance;
	heldItemInstance = nullptr;
}

void EntityZombie::entityInit()
{
	EntityMob::entityInit();
	dataWatcher->addObject(12, static_cast<byte_t>(0));
	dataWatcher->addObject(13, static_cast<byte_t>(0));
	dataWatcher->addObject(14, static_cast<byte_t>(0));
}

void EntityZombie::onUpdate()
{
	if (!worldObj->multiplayerWorld && isConverting())
	{
		int_t boost = getConversionTimeBoost();
		conversionTime -= boost;
		if (conversionTime <= 0)
		{
			convertToVillager();
		}
	}
	EntityMob::onUpdate();
}

void EntityZombie::onLivingUpdate()
{
	if (worldObj->isDaytime() && !worldObj->multiplayerWorld && !isChild())
	{
		float f = getEntityBrightness(1.0f);
		if (f > 0.5f && worldObj->canBlockSeeTheSky(MathHelper::floor_double(posX), MathHelper::floor_double(posY), MathHelper::floor_double(posZ)) && rand.nextFloat() * 30.0f < (f - 0.4f) * 2.0f)
		{
			setFire(8);
		}
	}

	if (!worldObj->multiplayerWorld && canPickUpLoot && !isDead && heldItemInstance == nullptr && (ticksExisted % 20 == 0))
	{
		AxisAlignedBB *aabb = boundingBox->expand(1.0, 0.0, 1.0);
		std::vector<Entity *> entities = worldObj->getEntitiesWithinAABB(typeid(EntityItem), aabb);
		for (Entity *ent : entities)
		{
			EntityItem *itemEnt = dynamic_cast<EntityItem *>(ent);
			if (itemEnt != nullptr && !itemEnt->isDead && itemEnt->item != nullptr)
			{
				heldItemInstance = new ItemStack(*itemEnt->item);
				itemEnt->setDead();
				break;
			}
		}
	}

	EntityMob::onLivingUpdate();
}

bool EntityZombie::isAIEnabled()
{
	return true;
}

int_t EntityZombie::getMaxHealth() const
{
	return 20;
}

int_t EntityZombie::getTotalArmorValue() const
{
	return 2;
}

float EntityZombie::getSpeedModifier()
{
	return EntityMob::getSpeedModifier() * (isChild() ? 1.5f : 1.0f);
}

const char *EntityZombie::getEntityTexture()
{
	return isVillager() ? "/mob/zombie_villager.png" : "/mob/zombie.png";
}

bool EntityZombie::isChild()
{
	return dataWatcher->getWatchableObjectByte(12) == 1;
}

void EntityZombie::setChild(bool child)
{
	dataWatcher->updateObject(12, static_cast<byte_t>(child ? 1 : 0));
}

bool EntityZombie::isVillager() const
{
	return dataWatcher->getWatchableObjectByte(13) == 1;
}

void EntityZombie::setVillager(bool villager)
{
	dataWatcher->updateObject(13, static_cast<byte_t>(villager ? 1 : 0));
}

bool EntityZombie::isConverting() const
{
	return dataWatcher->getWatchableObjectByte(14) == 1;
}

void EntityZombie::startConversion(int_t time)
{
	conversionTime = time;
	dataWatcher->updateObject(14, static_cast<byte_t>(1));
	removePotionEffect(Potion::weakness->id);
	addPotionEffect(new PotionEffect(Potion::damageBoost->id, time, std::min(worldObj->difficultySetting - 1, 0)));
	worldObj->setEntityState(this, static_cast<byte_t>(16));
}

void EntityZombie::convertToVillager()
{
	EntityVillager *villager = new EntityVillager(worldObj);
	villager->setLocationAndAngles(posX, posY, posZ, rotationYaw, rotationPitch);
	villager->initCreature();
	if (isChild())
	{
		villager->setGrowingAge(-24000);
	}
	setDead();
	worldObj->spawnEntityInWorld(villager);
	villager->addPotionEffect(new PotionEffect(Potion::confusion->id, 200, 0));
	worldObj->playAuxSFXAtEntity(nullptr, 1017, static_cast<int_t>(posX), static_cast<int_t>(posY), static_cast<int_t>(posZ), 0);
}

int_t EntityZombie::getConversionTimeBoost()
{
	int_t boost = 1;
	if (rand.nextFloat() < 0.01f)
	{
		int_t count = 0;
		for (int_t x = static_cast<int_t>(posX) - 4; x < static_cast<int_t>(posX) + 4 && count < 14; ++x)
		{
			for (int_t y = static_cast<int_t>(posY) - 4; y < static_cast<int_t>(posY) + 4 && count < 14; ++y)
			{
				for (int_t z = static_cast<int_t>(posZ) - 4; z < static_cast<int_t>(posZ) + 4 && count < 14; ++z)
				{
					int_t blockId = worldObj->getBlockId(x, y, z);
					if ((Block::fenceIron != nullptr && blockId == Block::fenceIron->blockID) ||
						(Block::blockBed != nullptr && blockId == Block::blockBed->blockID))
					{
						if (rand.nextFloat() < 0.3f)
						{
							boost++;
						}
						count++;
					}
				}
			}
		}
	}
	return boost;
}

void EntityZombie::initCreature()
{
	if (worldObj->rand.nextFloat() < 0.05f)
	{
		setVillager(true);
	}
	if (worldObj->rand.nextFloat() < 0.05f)
	{
		setChild(true);
	}
	if (rand.nextFloat() < (worldObj->difficultySetting == 3 ? 0.05f : 0.01f))
	{
		int_t r = rand.nextInt(3);
		if (r == 0)
		{
			heldItemInstance = new ItemStack(Item::swordSteel);
		}
		else
		{
			heldItemInstance = new ItemStack(Item::shovelSteel);
		}
	}
	if (rand.nextFloat() < (worldObj->difficultySetting == 3 ? 0.15f : (worldObj->difficultySetting == 2 ? 0.05f : 0.0f)))
	{
		canPickUpLoot = true;
	}
}

bool EntityZombie::interact(EntityPlayer *entityPlayer)
{
	ItemStack *itemstack = entityPlayer->inventory->getCurrentItem();
	if (itemstack != nullptr && Item::appleGold != nullptr && itemstack->getItem() == Item::appleGold &&
		itemstack->getItemDamage() == 0 && isVillager() && Potion::weakness != nullptr && isPotionActive(Potion::weakness))
	{
		if (!entityPlayer->capabilities.isCreativeMode)
		{
			itemstack->stackSize--;
		}
		if (itemstack->stackSize <= 0)
		{
			entityPlayer->inventory->setInventorySlotContents(entityPlayer->inventory->currentItem, nullptr);
		}
		if (!worldObj->multiplayerWorld)
		{
			startConversion(rand.nextInt(2401) + 3600);
		}
		return true;
	}
	return false;
}

void EntityZombie::onKillEntity(EntityLiving *entityLiving)
{
	EntityMob::onKillEntity(entityLiving);
	if (worldObj->difficultySetting >= 2 && dynamic_cast<EntityVillager *>(entityLiving) != nullptr)
	{
		if (worldObj->difficultySetting == 2 && rand.nextBoolean())
		{
			return;
		}
		EntityZombie *zombie = new EntityZombie(worldObj);
		zombie->setLocationAndAngles(entityLiving->posX, entityLiving->posY, entityLiving->posZ, entityLiving->rotationYaw, entityLiving->rotationPitch);
		entityLiving->setDead();
		zombie->initCreature();
		zombie->setVillager(true);
		if (entityLiving->isChild())
		{
			zombie->setChild(true);
		}
		worldObj->spawnEntityInWorld(zombie);
		worldObj->playAuxSFXAtEntity(nullptr, 1016, static_cast<int_t>(posX), static_cast<int_t>(posY), static_cast<int_t>(posZ), 0);
	}
}

void EntityZombie::handleHealthUpdate(byte_t byte0)
{
	if (byte0 == 16)
	{
		worldObj->playSoundEffect(posX + 0.5, posY + 0.5, posZ + 0.5, "mob.zombie.remedy", 1.0f + rand.nextFloat(), rand.nextFloat() * 0.7f + 0.3f);
	}
	else
	{
		EntityMob::handleHealthUpdate(byte0);
	}
}

bool EntityZombie::attackEntityAsMob(Entity *entity)
{
	bool result = EntityMob::attackEntityAsMob(entity);
	if (result && getHeldItem() == nullptr && isBurning() && rand.nextFloat() < static_cast<float>(worldObj->difficultySetting) * 0.3f)
	{
		entity->setFire(2 * worldObj->difficultySetting);
	}
	return result;
}

void EntityZombie::onDeath(const DamageSource &source)
{
	EntityMob::onDeath(source);
	if (heldItemInstance != nullptr)
	{
		entityDropItem(heldItemInstance, 0.0f);
		heldItemInstance = nullptr;
	}
}

ItemStack *EntityZombie::getHeldItem()
{
	return heldItemInstance;
}

jstring EntityZombie::getLivingSound() { return "mob.zombie"; }
jstring EntityZombie::getHurtSound()   { return "mob.zombiehurt"; }
jstring EntityZombie::getDeathSound()  { return "mob.zombiedeath"; }

int_t EntityZombie::getDropItemId()
{
	return Item::rottenFlesh != nullptr ? Item::rottenFlesh->shiftedIndex : 0;
}

EnumCreatureAttribute EntityZombie::getCreatureAttribute() const
{
	return EnumCreatureAttribute::UNDEAD;
}

void EntityZombie::dropRareDrop(int_t)
{
	switch (rand.nextInt(3))
	{
	case 0: if (Item::ingotIron != nullptr) dropItem(Item::ingotIron->shiftedIndex, 1); break;
	case 1: if (Item::carrot != nullptr) dropItem(Item::carrot->shiftedIndex, 1); break;
	case 2: if (Item::potato != nullptr) dropItem(Item::potato->shiftedIndex, 1); break;
	}
}

void EntityZombie::writeEntityToNBT(NBTTagCompound *tag)
{
	EntityMob::writeEntityToNBT(tag);
	if (isChild())
	{
		tag->setBoolean("IsBaby", true);
	}
	if (isVillager())
	{
		tag->setBoolean("IsVillager", true);
	}
	tag->setInteger("ConversionTime", isConverting() ? conversionTime : -1);
}

void EntityZombie::readEntityFromNBT(NBTTagCompound *tag)
{
	EntityMob::readEntityFromNBT(tag);
	if (tag->getBoolean("IsBaby"))
	{
		setChild(true);
	}
	if (tag->getBoolean("IsVillager"))
	{
		setVillager(true);
	}
	if (tag->hasKey("ConversionTime") && tag->getInteger("ConversionTime") > -1)
	{
		startConversion(tag->getInteger("ConversionTime"));
	}
}
