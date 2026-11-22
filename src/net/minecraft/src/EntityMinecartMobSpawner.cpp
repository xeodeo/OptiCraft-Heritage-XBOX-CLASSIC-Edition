#include "EntityMinecartMobSpawner.h"

#include "MathHelper.h"
#include "NBTTagCompound.h"
#include "World.h"

EntityMinecartMobSpawnerLogic::EntityMinecartMobSpawnerLogic(EntityMinecartMobSpawner *cart)
	: spawnerMinecart(cart)
{
}

void EntityMinecartMobSpawnerLogic::broadcastEvent(int_t eventId)
{
	(void)eventId;
}

World *EntityMinecartMobSpawnerLogic::getSpawnerWorld()
{
	return spawnerMinecart != nullptr ? spawnerMinecart->worldObj : nullptr;
}

int_t EntityMinecartMobSpawnerLogic::getSpawnerX()
{
	return spawnerMinecart != nullptr ? MathHelper::floor_double(spawnerMinecart->posX) : 0;
}

int_t EntityMinecartMobSpawnerLogic::getSpawnerY()
{
	return spawnerMinecart != nullptr ? MathHelper::floor_double(spawnerMinecart->posY) : 0;
}

int_t EntityMinecartMobSpawnerLogic::getSpawnerZ()
{
	return spawnerMinecart != nullptr ? MathHelper::floor_double(spawnerMinecart->posZ) : 0;
}

EntityMinecartMobSpawner::EntityMinecartMobSpawner(World *world)
	: EntityMinecart(world)
	, mobSpawnerLogic(this)
{
	minecartType = 4;
}

EntityMinecartMobSpawner::EntityMinecartMobSpawner(World *world, double x, double y, double z)
	: EntityMinecart(world, x, y, z, 4)
	, mobSpawnerLogic(this)
{
}

void EntityMinecartMobSpawner::onUpdate()
{
	EntityMinecart::onUpdate();
	mobSpawnerLogic.updateSpawner();
}

void EntityMinecartMobSpawner::readEntityFromNBT(NBTTagCompound *nbttagcompound)
{
	EntityMinecart::readEntityFromNBT(nbttagcompound);
	mobSpawnerLogic.readFromNBT(nbttagcompound);
}

void EntityMinecartMobSpawner::writeEntityToNBT(NBTTagCompound *nbttagcompound)
{
	EntityMinecart::writeEntityToNBT(nbttagcompound);
	mobSpawnerLogic.writeToNBT(nbttagcompound);
}
