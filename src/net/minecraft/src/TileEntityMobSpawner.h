#pragma once

#include "MobSpawnerBaseLogic.h"
#include "TileEntity.h"
#include <string>

class NBTTagCompound;
class TileEntityMobSpawner;

class TileEntityMobSpawnerLogic : public MobSpawnerBaseLogic
{
public:
	explicit TileEntityMobSpawnerLogic(TileEntityMobSpawner *owner);

	void broadcastEvent(int_t eventId) override;
	World *getSpawnerWorld() override;
	int_t getSpawnerX() override;
	int_t getSpawnerY() override;
	int_t getSpawnerZ() override;

private:
	TileEntityMobSpawner *mobSpawnerEntity;
};

class TileEntityMobSpawner : public TileEntity
{
public:
	TileEntityMobSpawner();

	std::string getMobID();
	void setMobID(const std::string &s);
	bool anyPlayerInRange();
	MobSpawnerBaseLogic *getSpawnerLogic() { return &spawnerLogic; }

	void readFromNBT(NBTTagCompound *nbttagcompound) override;
	void writeToNBT(NBTTagCompound *nbttagcompound) override;
	void updateEntity() override;

private:
	TileEntityMobSpawnerLogic spawnerLogic;
};
