#pragma once

#include <string>
#include <vector>
#include "java/Type.h"

class Entity;
class NBTTagCompound;
class World;

struct WeightedSpawnPotential
{
	int_t weight;
	std::string type;
	NBTTagCompound *properties;

	WeightedSpawnPotential(int_t w, const std::string &t, NBTTagCompound *props);
	WeightedSpawnPotential(const WeightedSpawnPotential &other);
	WeightedSpawnPotential &operator=(const WeightedSpawnPotential &other);
	~WeightedSpawnPotential();
};

// net.minecraft.src.MobSpawnerBaseLogic (Java 1.5.2)
class MobSpawnerBaseLogic
{
public:
	MobSpawnerBaseLogic();
	virtual ~MobSpawnerBaseLogic();

	std::string getEntityNameToSpawn() const;
	void setMobID(const std::string &id);
	bool canRun();
	void updateSpawner();
	void readFromNBT(NBTTagCompound *nbt);
	void writeToNBT(NBTTagCompound *nbt);
	Entity *getCachedEntity();
	bool setDelayToMin(int_t eventId);

	virtual void broadcastEvent(int_t eventId) = 0;
	virtual World *getSpawnerWorld() = 0;
	virtual int_t getSpawnerX() = 0;
	virtual int_t getSpawnerY() = 0;
	virtual int_t getSpawnerZ() = 0;

public:
	int_t spawnDelay;
	double yaw;
	double yaw2;
	int_t minSpawnDelay;
	int_t maxSpawnDelay;
	int_t spawnCount;
	int_t maxNearbyEntities;
	int_t activatingRangeFromPlayer;
	int_t spawnRange;

private:
	Entity *spawnEntityWithData(Entity *entity);
	void resetTimer();
	void clearCachedEntity();
	void setRandomMinecart(const WeightedSpawnPotential *entry);

	std::string mobID;
	std::vector<WeightedSpawnPotential> potentialSpawns;
	WeightedSpawnPotential *randomMinecart;
	Entity *cachedEntity;
};
