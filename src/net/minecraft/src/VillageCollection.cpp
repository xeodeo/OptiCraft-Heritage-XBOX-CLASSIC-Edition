#include "VillageCollection.h"

#include <string>

#include <cmath>
#include <limits>

#include "Block.h"
#include "BlockDoor.h"
#include "Entity.h"
#include "EntityVillager.h"
#include "Village.h"
#include "VillageDoorInfo.h"
#include "NBTTagCompound.h"
#include "NBTTagList.h"
#include "MapDataBase.h"
#include "MapStorage.h"
#include "World.h"
#include "WorldProvider.h"
#include "java/Arithmetic.h"

namespace
{
MapDataBase *createVillageData(const std::string &name)
{
	return new VillageCollection(name);
}
}

VillageCollection::VillageCollection(World *world) : MapDataBase("villages"), worldObj(world), tickCounter(0)
{
}

void VillageCollection::loadSavedVillages()
{
	// Java registers the collection in MapStorage; here the World owns it (it deletes it and
	// saves it), so only the villages.dat copy is read and then freed. villages.dat belongs to
	// the overworld: the Nether/End must not read or overwrite it.
	if (worldObj != nullptr && worldObj->getMapStorage() != nullptr && worldObj->worldProvider != nullptr &&
		worldObj->worldProvider->worldType == 0)
	{
		MapStorage *storage = worldObj->getMapStorage();
		MapDataBase *saved = storage->loadData("villages", &createVillageData);
		if (auto *loaded = dynamic_cast<VillageCollection *>(saved))
		{
			NBTTagCompound data;
			loaded->writeToNBT(&data);
			readFromNBT(&data);
		}
		if (saved != nullptr && storage->takeData("villages") == saved)
			delete saved;
	}
}

VillageCollection::VillageCollection(const std::string &name) : MapDataBase(name), worldObj(nullptr), tickCounter(0) {}

VillageCollection::~VillageCollection() = default;

void VillageCollection::addVillagerPosition(int_t x, int_t y, int_t z)
{
	if (villagerPositionsList.size() <= 64 && !isVillagerPositionPresent(x, y, z))
		villagerPositionsList.emplace_back(x, y, z);
}

void VillageCollection::tick()
{
	// The provider's dimension is only known after World registers it, so load on the first tick.
	if (!savedVillagesLoaded)
	{
		savedVillagesLoaded = true;
		loadSavedVillages();
	}
	tickCounter = JavaArithmetic::intAdd(tickCounter, 1);
	for (const auto &village : villageList)
		village->tick(tickCounter);
	removeAnnihilatedVillages();
	dropOldestVillagerPosition();
	addNewDoorsToVillageOrCreateVillage();
	if (tickCounter % 400 == 0)
		markDirty();
}

void VillageCollection::readFromNBT(NBTTagCompound *tag)
{
	if (tag == nullptr) return;
	tickCounter = tag->getInteger("Tick");
	villageList.clear();
	if (tag->hasKey("Villages"))
	{
		NBTTagList *villages = tag->getTagList("Villages");
		for (int_t i = 0; i < villages->tagCount(); ++i)
		{
			auto *entry = dynamic_cast<NBTTagCompound *>(villages->tagAt(i));
			if (entry == nullptr) continue;
			std::unique_ptr<Village> village(new Village(worldObj));
			village->readVillageDataFromNBT(entry);
			villageList.push_back(std::move(village));
		}
	}
}

void VillageCollection::writeToNBT(NBTTagCompound *tag)
{
	if (tag == nullptr) return;
	tag->setInteger("Tick", tickCounter);
	NBTTagList *villages = new NBTTagList();
	for (const auto &village : villageList)
	{
		NBTTagCompound *entry = new NBTTagCompound();
		village->writeVillageDataToNBT(entry);
		villages->appendTag(entry);
	}
	tag->setTag("Villages", villages);
}

const std::vector<std::unique_ptr<Village>> &VillageCollection::getVillageList() const
{
	return villageList;
}

Village *VillageCollection::findNearestVillage(int_t x, int_t y, int_t z, int_t radius) const
{
	Village *best = nullptr;
	float bestDistance = std::numeric_limits<float>::max();
	for (const auto &village : villageList)
	{
		ChunkCoordinates center = village->getCenter();
		float distance = center.getDistanceSquared(x, y, z);
		if (distance < bestDistance)
		{
			const int_t allowed = JavaArithmetic::intAdd(radius, village->getVillageRadius());
			if (distance <= static_cast<float>(JavaArithmetic::intMul(allowed, allowed)))
			{
				best = village.get();
				bestDistance = distance;
			}
		}
	}
	return best;
}

void VillageCollection::onEntityRemoved(Entity *entity)
{
	for (const auto &village : villageList)
		village->onEntityRemoved(entity);
}

void VillageCollection::removeAnnihilatedVillages()
{
	for (auto it = villageList.begin(); it != villageList.end();)
	{
		if ((*it)->isAnnihilated())
		{
			// Java villagers keep the dead Village alive until their next lookup; here it is freed.
			if (worldObj != nullptr)
				for (Entity *entity : worldObj->loadedEntityList)
					if (auto *villager = dynamic_cast<EntityVillager *>(entity))
						villager->forgetVillage(it->get());
			it = villageList.erase(it);
		}
		else
			++it;
	}
}

void VillageCollection::dropOldestVillagerPosition()
{
	if (villagerPositionsList.empty())
		return;
	ChunkCoordinates position = villagerPositionsList.front();
	villagerPositionsList.erase(villagerPositionsList.begin());
	addUnassignedWoodenDoorsAroundToNewDoorsList(position);
}

void VillageCollection::addNewDoorsToVillageOrCreateVillage()
{
	for (auto &ownedDoor : newDoors)
	{
		VillageDoorInfo *door = ownedDoor.get();
		bool assigned = false;
		for (const auto &village : villageList)
		{
			ChunkCoordinates center = village->getCenter();
			int_t distance = JavaArithmetic::doubleToInt(
				center.getEuclideanDistanceTo(door->posX, door->posY, door->posZ));
			if (distance <= JavaArithmetic::intAdd(32, village->getVillageRadius()))
			{
				village->addVillageDoorInfo(ownedDoor.release());
				assigned = true;
				break;
			}
		}
		if (!assigned)
		{
			std::unique_ptr<Village> village(new Village(worldObj));
			village->addVillageDoorInfo(ownedDoor.release());
			villageList.push_back(std::move(village));
		}
	}
	newDoors.clear();
}

void VillageCollection::addUnassignedWoodenDoorsAroundToNewDoorsList(const ChunkCoordinates &position)
{
	const int_t minX = JavaArithmetic::intSub(position.x, 16);
	const int_t maxX = JavaArithmetic::intAdd(position.x, 16);
	const int_t minY = JavaArithmetic::intSub(position.y, 4);
	const int_t maxY = JavaArithmetic::intAdd(position.y, 4);
	const int_t minZ = JavaArithmetic::intSub(position.z, 16);
	const int_t maxZ = JavaArithmetic::intAdd(position.z, 16);
	for (int_t x = minX; x < maxX; ++x)
		for (int_t y = minY; y < maxY; ++y)
			for (int_t z = minZ; z < maxZ; ++z)
			{
				if (!isWoodenDoorAt(x, y, z))
					continue;
				VillageDoorInfo *door = getVillageDoorAt(x, y, z);
				if (door == nullptr)
					addDoorToNewListIfAppropriate(x, y, z);
				else
					door->lastActivityTimestamp = tickCounter;
			}
}

VillageDoorInfo *VillageCollection::getVillageDoorAt(int_t x, int_t y, int_t z) const
{
	for (const auto &door : newDoors)
		if (door->posX == x && door->posZ == z && JavaArithmetic::intAbs(JavaArithmetic::intSub(door->posY, y)) <= 1)
			return door.get();
	for (const auto &village : villageList)
	{
		VillageDoorInfo *door = village->getVillageDoorAt(x, y, z);
		if (door != nullptr)
			return door;
	}
	return nullptr;
}

void VillageCollection::addDoorToNewListIfAppropriate(int_t x, int_t y, int_t z)
{
	if (worldObj == nullptr || Block::doorWood == nullptr)
		return;
	BlockDoor *doorBlock = dynamic_cast<BlockDoor *>(Block::doorWood);
	if (doorBlock == nullptr)
		return;
	int_t orientation = doorBlock->getDoorOrientation(worldObj, x, y, z);
	int_t skyScore = 0;
	if (orientation != 0 && orientation != 2)
	{
		for (int_t offset = -5; offset < 0; ++offset)
			if (worldObj->canBlockSeeTheSky(x, y, JavaArithmetic::intAdd(z, offset))) --skyScore;
		for (int_t offset = 1; offset <= 5; ++offset)
			if (worldObj->canBlockSeeTheSky(x, y, JavaArithmetic::intAdd(z, offset))) ++skyScore;
		if (skyScore != 0)
			newDoors.emplace_back(new VillageDoorInfo(x, y, z, 0, skyScore > 0 ? -2 : 2, tickCounter));
	}
	else
	{
		for (int_t offset = -5; offset < 0; ++offset)
			if (worldObj->canBlockSeeTheSky(JavaArithmetic::intAdd(x, offset), y, z)) --skyScore;
		for (int_t offset = 1; offset <= 5; ++offset)
			if (worldObj->canBlockSeeTheSky(JavaArithmetic::intAdd(x, offset), y, z)) ++skyScore;
		if (skyScore != 0)
			newDoors.emplace_back(new VillageDoorInfo(x, y, z, skyScore > 0 ? -2 : 2, 0, tickCounter));
	}
}

bool VillageCollection::isVillagerPositionPresent(int_t x, int_t y, int_t z) const
{
	for (const ChunkCoordinates &position : villagerPositionsList)
		if (position.x == x && position.y == y && position.z == z)
			return true;
	return false;
}

bool VillageCollection::isWoodenDoorAt(int_t x, int_t y, int_t z) const
{
	return worldObj != nullptr && Block::doorWood != nullptr && worldObj->getBlockId(x, y, z) == Block::doorWood->blockID;
}
