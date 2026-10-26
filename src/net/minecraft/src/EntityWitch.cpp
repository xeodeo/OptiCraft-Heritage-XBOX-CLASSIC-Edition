#include "EntityWitch.h"

#include <vector>

#include "AxisAlignedBB.h"
#include "DamageSource.h"
#include "DataWatcher.h"
#include "EntityAIArrowAttack.h"
#include "EntityAIHurtByTarget.h"
#include "EntityAILookIdle.h"
#include "EntityAINearestAttackableTarget.h"
#include "EntityAISwimming.h"
#include "EntityAIWander.h"
#include "EntityAIWatchClosest.h"
#include "EntityPlayer.h"
#include "EntityPotion.h"
#include "Item.h"
#include "ItemPotion.h"
#include "MathHelper.h"
#include "Potion.h"
#include "World.h"

EntityWitch::~EntityWitch()
{
	delete heldPotion;
	heldPotion = nullptr;
}

EntityWitch::EntityWitch(World *world)
	: EntityMob(world)
{
	ensureEntityInit();
	texture = "/mob/villager/witch.png";
	moveSpeed = 0.25f;

	tasks.addTask(1, new EntityAISwimming(this));
	tasks.addTask(2, new EntityAIArrowAttack(this, moveSpeed, 60, 10.0f));
	tasks.addTask(2, new EntityAIWander(this, moveSpeed));
	tasks.addTask(3, new EntityAIWatchClosest(this, typeid(EntityPlayer), 8.0f));
	tasks.addTask(3, new EntityAILookIdle(this));

	targetTasks.addTask(1, new EntityAIHurtByTarget(this, false));
	targetTasks.addTask(2, new EntityAINearestAttackableTarget(this, typeid(EntityPlayer), 16.0f, 0, true));
}

void EntityWitch::entityInit()
{
	EntityMob::entityInit();
	dataWatcher->addObject(21, static_cast<byte_t>(0));
}

void EntityWitch::setAggressive(bool flag)
{
	dataWatcher->updateObject(21, static_cast<byte_t>(flag ? 1 : 0));
}

bool EntityWitch::getAggressive() const
{
	return dataWatcher->getWatchableObjectByte(21) == 1;
}

float EntityWitch::getSpeedModifier()
{
	float speed = EntityMob::getSpeedModifier();
	if (getAggressive())
		speed *= 0.75f;
	return speed;
}

ItemStack *EntityWitch::getHeldItem()
{
	return heldPotion;
}

void EntityWitch::onLivingUpdate()
{
	if (!worldObj->multiplayerWorld)
	{
		if (getAggressive())
		{
			if (witchAttackTimer-- <= 0)
			{
				setAggressive(false);
				if (heldPotion != nullptr && Item::potion != nullptr)
				{
					ItemPotion *itemPot = dynamic_cast<ItemPotion *>(Item::potion);
					if (itemPot != nullptr)
					{
						const auto &effects = itemPot->getEffects(heldPotion);
						for (const auto &eff : effects)
						{
							addPotionEffect(new PotionEffect(eff));
						}
					}
				}
				delete heldPotion;
				heldPotion = nullptr;
			}
		}
		else
		{
			short potionDamage = -1;
			if (rand.nextFloat() < 0.15f && isBurning() && (Potion::fireResistance != nullptr && !isPotionActive(Potion::fireResistance)))
			{
				potionDamage = 16307;
			}
			else if (rand.nextFloat() < 0.05f && health < getMaxHealth())
			{
				potionDamage = 16341;
			}
			else if (rand.nextFloat() < 0.25f && getAttackTarget() != nullptr &&
			         (Potion::moveSpeed != nullptr && !isPotionActive(Potion::moveSpeed)) &&
			         getAttackTarget()->getDistanceSqToEntity(this) > 121.0)
			{
				potionDamage = 16274;
			}

			if (potionDamage > -1 && Item::potion != nullptr)
			{
				delete heldPotion;
				heldPotion = new ItemStack(Item::potion, 1, potionDamage);
				witchAttackTimer = Item::potion->getMaxItemUseDuration(heldPotion);
				setAggressive(true);
			}
		}

		if (rand.nextFloat() < 7.5e-4f)
		{
			worldObj->setEntityState(this, static_cast<byte_t>(15));
		}
	}

	EntityMob::onLivingUpdate();
}

void EntityWitch::handleHealthUpdate(byte_t byte0)
{
	if (byte0 == 15)
	{
		const int_t count = rand.nextInt(35) + 10;
		for (int_t i = 0; i < count; ++i)
		{
			worldObj->spawnParticle(
				"witchMagic",
				posX + rand.nextGaussian() * 0.13,
				boundingBox->maxY + 0.5 + rand.nextGaussian() * 0.13,
				posZ + rand.nextGaussian() * 0.13,
				0.0, 0.0, 0.0
			);
		}
	}
	else
	{
		EntityMob::handleHealthUpdate(byte0);
	}
}

int_t EntityWitch::applyPotionDamageCalculations(const DamageSource &source, int_t damage)
{
	damage = EntityMob::applyPotionDamageCalculations(source, damage);
	if (source.getEntity() == this)
		damage = 0;
	if (source.isMagicDamage())
		damage = static_cast<int_t>(static_cast<double>(damage) * 0.15);
	return damage;
}

void EntityWitch::dropFewItems(bool recentlyHitByPlayer, int_t lootingLevel)
{
	(void)recentlyHitByPlayer;
	static const int_t witchDrops[] = {
		Item::lightStoneDust != nullptr ? Item::lightStoneDust->shiftedIndex : 348,
		Item::sugar != nullptr ? Item::sugar->shiftedIndex : 353,
		Item::redstone != nullptr ? Item::redstone->shiftedIndex : 331,
		Item::spiderEye != nullptr ? Item::spiderEye->shiftedIndex : 375,
		Item::glassBottle != nullptr ? Item::glassBottle->shiftedIndex : 374,
		Item::gunpowder != nullptr ? Item::gunpowder->shiftedIndex : 289,
		Item::stick != nullptr ? Item::stick->shiftedIndex : 280,
		Item::stick != nullptr ? Item::stick->shiftedIndex : 280
	};
	constexpr int_t numDrops = sizeof(witchDrops) / sizeof(witchDrops[0]);

	const int_t dropCount = rand.nextInt(3) + 1;
	for (int_t i = 0; i < dropCount; ++i)
	{
		int_t stackSize = rand.nextInt(3);
		const int_t itemId = witchDrops[rand.nextInt(numDrops)];
		if (lootingLevel > 0)
			stackSize += rand.nextInt(lootingLevel + 1);

		for (int_t j = 0; j < stackSize; ++j)
			dropItem(itemId, 1);
	}
}

void EntityWitch::attackEntityWithRangedAttack(EntityLiving *target, float)
{
	if (!getAggressive() && target != nullptr)
	{
		EntityPotion *potion = new EntityPotion(worldObj, this, 32732);
		potion->rotationPitch -= -20.0f;

		const double dx = target->posX + target->motionX - posX;
		const double dy = target->posY + static_cast<double>(target->getEyeHeight()) - 1.1 - posY;
		const double dz = target->posZ + target->motionZ - posZ;
		const float dist = MathHelper::sqrt_double(dx * dx + dz * dz);

		if (dist >= 8.0f && (Potion::moveSlowdown != nullptr && !target->isPotionActive(Potion::moveSlowdown)))
		{
			potion->setPotionDamage(32698);
		}
		else if (target->getHealth() >= 8 && (Potion::poison != nullptr && !target->isPotionActive(Potion::poison)))
		{
			potion->setPotionDamage(32660);
		}
		else if (dist <= 3.0f && (Potion::weakness != nullptr && !target->isPotionActive(Potion::weakness)) && rand.nextFloat() < 0.25f)
		{
			potion->setPotionDamage(32696);
		}

		potion->setThrowableHeading(dx, dy + static_cast<double>(dist * 0.2f), dz, 0.75f, 8.0f);
		if (!worldObj->entityJoinedWorld(potion))
			delete potion;
	}
}
