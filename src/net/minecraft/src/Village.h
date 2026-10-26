#pragma once

#include <memory>
#include <map>
#include <string>
#include <vector>

#include "ChunkCoordinates.h"
#include "java/String.h"
#include "VillageAgressor.h"
#include "java/Type.h"

class Entity;
class EntityLiving;
class VillageDoorInfo;
class NBTTagCompound;
class EntityPlayer;
class World;

// net.minecraft.src.Village
class Village
{
public:
	explicit Village(World *world);
	~Village();

	void tick(int_t tickCounter);
	ChunkCoordinates getCenter() const;
	int_t getVillageRadius() const;
	int_t getNumVillageDoors() const;
	int_t getTicksSinceLastDoorAdding() const;
	int_t getNumVillagers() const;
	bool isInRange(int_t x, int_t y, int_t z) const;
	const std::vector<std::unique_ptr<VillageDoorInfo>> &getVillageDoorInfoList() const;
	VillageDoorInfo *findNearestDoor(int_t x, int_t y, int_t z) const;
	VillageDoorInfo *findNearestDoorUnrestricted(int_t x, int_t y, int_t z) const;
	VillageDoorInfo *getVillageDoorAt(int_t x, int_t y, int_t z) const;
	void addVillageDoorInfo(VillageDoorInfo *door);
	bool isAnnihilated() const;
	void addOrRenewAgressor(EntityLiving *aggressor);
	EntityLiving *findNearestVillageAggressor(EntityLiving *target) const;
	void onEntityRemoved(Entity *entity);
	int_t getReputationForPlayer(const jstring &name) const;
	int_t setReputationForPlayer(const jstring &name, int_t delta);
	bool isPlayerReputationTooLow(const jstring &name) const;
	void modifyPlayerReputation(int_t delta);
	EntityPlayer *findNearestPlayerWithNegativeReputation(EntityLiving *near) const;
	void endMatingSeason();
	bool isMatingSeason() const;
	void readVillageDataFromNBT(NBTTagCompound *tag);
	void writeVillageDataToNBT(NBTTagCompound *tag) const;

private:

	void updateNumIronGolems();
	void updateNumVillagers();
	void removeDeadAndOldAgressors();
	void removeDeadAndOutOfRangeDoors();
	bool isBlockDoor(int_t x, int_t y, int_t z) const;
	void updateVillageRadiusAndCenter();
	bool trySpawnIronGolem();
	bool isValidIronGolemSpawningLocation(int_t x, int_t y, int_t z, int_t sizeX, int_t sizeY, int_t sizeZ) const;

	World *worldObj;
	std::vector<std::unique_ptr<VillageDoorInfo>> villageDoorInfoList;
	ChunkCoordinates centerHelper;
	ChunkCoordinates center;
	int_t villageRadius;
	int_t lastAddDoorTimestamp;
	int_t tickCounter;
	int_t numVillagers;
	std::vector<VillageAgressor> villageAgressors;
	int_t numIronGolems;
	int_t noBreedTicks;
	std::map<jstring, int_t> playerReputation;
};
