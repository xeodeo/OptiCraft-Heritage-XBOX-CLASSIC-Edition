#include "EntityWitherSkull.h"

#include <algorithm>

#include "Block.h"
#include "DamageSource.h"
#include "DataWatcher.h"
#include "EntityLiving.h"
#include "MovingObjectPosition.h"
#include "Potion.h"
#include "PotionEffect.h"
#include "World.h"

EntityWitherSkull::EntityWitherSkull(World *world)
	: EntityFireball(world)
{
	ensureEntityInit();
	setSize(0.3125f, 0.3125f);
}

EntityWitherSkull::EntityWitherSkull(World *world, EntityLiving *shooter, double xAcceleration, double yAcceleration,
	double zAcceleration)
	: EntityFireball(world, shooter, xAcceleration, yAcceleration, zAcceleration)
{
	ensureEntityInit();
	setSize(0.3125f, 0.3125f);
}

EntityWitherSkull::EntityWitherSkull(World *world, double x, double y, double z, double xAcceleration,
	double yAcceleration, double zAcceleration)
	: EntityFireball(world, x, y, z, xAcceleration, yAcceleration, zAcceleration)
{
	ensureEntityInit();
	setSize(0.3125f, 0.3125f);
}

void EntityWitherSkull::entityInit()
{
	dataWatcher->addObject(10, static_cast<byte_t>(0));
}

float EntityWitherSkull::getMotionFactor()
{
	return isInvulnerable() ? 0.73f : EntityFireball::getMotionFactor();
}

bool EntityWitherSkull::isBurning()
{
	return false;
}

float EntityWitherSkull::func_82146_a(Explosion *explosion, World *world, int_t x, int_t y, int_t z, Block *block)
{
	float resistance = EntityFireball::func_82146_a(explosion, world, x, y, z, block);
	if (isInvulnerable() && block != Block::bedrock && block != Block::endPortal && block != Block::endPortalFrame)
		resistance = std::min(0.8f, resistance);
	return resistance;
}

void EntityWitherSkull::onImpact(MovingObjectPosition *hit)
{
	if (worldObj->multiplayerWorld || hit == nullptr)
		return;

	if (hit->entityHit != nullptr)
	{
		EntityLiving *shooter = getShootingEntity();
		if (shooter != nullptr)
		{
			if (hit->entityHit->attackEntityFrom(DamageSource::causeMobDamage(shooter), 8) && !hit->entityHit->isEntityAlive())
				shooter->heal(5);
		}
		else
		{
			hit->entityHit->attackEntityFrom(DamageSource::magic, 5);
		}

		EntityLiving *living = dynamic_cast<EntityLiving *>(hit->entityHit);
		if (living != nullptr && Potion::wither != nullptr)
		{
			int_t seconds = 0;
			if (worldObj->difficultySetting == 2)
				seconds = 10;
			else if (worldObj->difficultySetting == 3)
				seconds = 40;
			if (seconds > 0)
				living->addPotionEffect(new PotionEffect(Potion::wither->id, 20 * seconds, 1));
		}
	}

	delete worldObj->newExplosion(this, posX, posY, posZ, 1.0f, false);
	setDead();
}

bool EntityWitherSkull::canBeCollidedWith()
{
	return false;
}

bool EntityWitherSkull::attackEntityFrom(Entity *entity, int_t damage)
{
	(void)entity;
	(void)damage;
	return false;
}

bool EntityWitherSkull::attackEntityFrom(const DamageSource &source, int_t damage)
{
	(void)source;
	(void)damage;
	return false;
}

bool EntityWitherSkull::isInvulnerable()
{
	return dataWatcher->getWatchableObjectByte(10) == 1;
}

void EntityWitherSkull::setInvulnerable(bool value)
{
	dataWatcher->updateObject(10, static_cast<byte_t>(value ? 1 : 0));
}

// Java keeps skulls whose shooter was not saved (after a reload) and they drift to a stop in mid-air.
// Skulls that do know their Wither already die with it; ownerless ones are removed after 10 s of flight.
void EntityWitherSkull::onUpdate()
{
	EntityFireball::onUpdate();
	if (!isDead && worldObj != nullptr && !worldObj->multiplayerWorld && !hasShooterRecord() && getTicksInAir() > 200)
		setEntityDead();
}
