#pragma once

#include "EntityMinecart.h"
#include "MobSpawnerBaseLogic.h"

class EntityMinecartMobSpawner;
class NBTTagCompound;
class World;

class EntityMinecartMobSpawnerLogic : public MobSpawnerBaseLogic
{
public:
	explicit EntityMinecartMobSpawnerLogic(EntityMinecartMobSpawner *cart);

	void broadcastEvent(int_t eventId) override;
	World *getSpawnerWorld() override;
	int_t getSpawnerX() override;
	int_t getSpawnerY() override;
	int_t getSpawnerZ() override;

private:
	EntityMinecartMobSpawner *spawnerMinecart;
};

// net.minecraft.src.EntityMinecartMobSpawner (Java 1.5.2, ID 47, minecartType 4)
class EntityMinecartMobSpawner : public EntityMinecart
{
public:
	explicit EntityMinecartMobSpawner(World *world);
	EntityMinecartMobSpawner(World *world, double x, double y, double z);

	int_t getMinecartType() const override { return 4; }
	MobSpawnerBaseLogic *getSpawnerLogic() { return &mobSpawnerLogic; }

	void onUpdate() override;
	void readEntityFromNBT(NBTTagCompound *nbttagcompound) override;
	void writeEntityToNBT(NBTTagCompound *nbttagcompound) override;

private:
	EntityMinecartMobSpawnerLogic mobSpawnerLogic;
};
