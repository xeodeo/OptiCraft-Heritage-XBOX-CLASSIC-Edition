#include "MobSpawnerBaseLogic.h"

#include "AxisAlignedBB.h"
#include "Entity.h"
#include "EntityList.h"
#include "EntityLiving.h"
#include "NBTTagCompound.h"
#include "NBTTagList.h"
#include "World.h"
#include "java/Random.h"

WeightedSpawnPotential::WeightedSpawnPotential(int_t w, const std::string &t, NBTTagCompound *props)
	: weight(w)
	, type(t)
	, properties(props != nullptr ? dynamic_cast<NBTTagCompound *>(props->copy()) : nullptr)
{
}

WeightedSpawnPotential::WeightedSpawnPotential(const WeightedSpawnPotential &other)
	: weight(other.weight)
	, type(other.type)
	, properties(other.properties != nullptr ? dynamic_cast<NBTTagCompound *>(other.properties->copy()) : nullptr)
{
}

WeightedSpawnPotential &WeightedSpawnPotential::operator=(const WeightedSpawnPotential &other)
{
	if (this != &other)
	{
		weight = other.weight;
		type = other.type;
		delete properties;
		properties = other.properties != nullptr ? dynamic_cast<NBTTagCompound *>(other.properties->copy()) : nullptr;
	}
	return *this;
}

WeightedSpawnPotential::~WeightedSpawnPotential()
{
	delete properties;
	properties = nullptr;
}

MobSpawnerBaseLogic::MobSpawnerBaseLogic()
	: spawnDelay(20)
	, yaw(0.0)
	, yaw2(0.0)
	, minSpawnDelay(200)
	, maxSpawnDelay(800)
	, spawnCount(4)
	, maxNearbyEntities(6)
	, activatingRangeFromPlayer(16)
	, spawnRange(4)
	, mobID("Pig")
	, potentialSpawns()
	, randomMinecart(nullptr)
	, cachedEntity(nullptr)
{
}

MobSpawnerBaseLogic::~MobSpawnerBaseLogic()
{
	delete randomMinecart;
	randomMinecart = nullptr;
	clearCachedEntity();
}

void MobSpawnerBaseLogic::clearCachedEntity()
{
	delete cachedEntity;
	cachedEntity = nullptr;
}

void MobSpawnerBaseLogic::setRandomMinecart(const WeightedSpawnPotential *entry)
{
	delete randomMinecart;
	randomMinecart = (entry != nullptr) ? new WeightedSpawnPotential(*entry) : nullptr;
	clearCachedEntity();
}

std::string MobSpawnerBaseLogic::getEntityNameToSpawn() const
{
	if (randomMinecart == nullptr)
	{
		if (mobID == "Minecart")
			return "MinecartRideable";
		return mobID;
	}
	return randomMinecart->type;
}

void MobSpawnerBaseLogic::setMobID(const std::string &id)
{
	if (mobID != id)
	{
		mobID = id;
		clearCachedEntity();
	}
}

bool MobSpawnerBaseLogic::canRun()
{
	World *world = getSpawnerWorld();
	if (world == nullptr)
		return false;
	return world->getClosestPlayer(
		(double)getSpawnerX() + 0.5,
		(double)getSpawnerY() + 0.5,
		(double)getSpawnerZ() + 0.5,
		(double)activatingRangeFromPlayer) != nullptr;
}

void MobSpawnerBaseLogic::updateSpawner()
{
	World *world = getSpawnerWorld();
	if (world == nullptr || !canRun())
		return;

	const int_t sx = getSpawnerX();
	const int_t sy = getSpawnerY();
	const int_t sz = getSpawnerZ();

	yaw2 = yaw;
	const double px = (double)sx + (double)world->rand.nextFloat();
	const double py = (double)sy + (double)world->rand.nextFloat();
	const double pz = (double)sz + (double)world->rand.nextFloat();
	world->spawnParticle("smoke", px, py, pz, 0.0, 0.0, 0.0);
	world->spawnParticle("flame", px, py, pz, 0.0, 0.0, 0.0);

	if (spawnDelay > 0 && world->multiplayerWorld)
	{
		--spawnDelay;
	}

	yaw += 1000.0 / ((double)spawnDelay + 200.0);
	while (yaw > 360.0)
	{
		yaw -= 360.0;
		yaw2 -= 360.0;
	}

	if (world->multiplayerWorld)
		return;

	if (spawnDelay == -1)
		resetTimer();

	if (spawnDelay > 0)
	{
		--spawnDelay;
		return;
	}

	bool spawnedAny = false;
	for (int_t i = 0; i < spawnCount; ++i)
	{
		Entity *entity = EntityList::createEntityInWorld(getEntityNameToSpawn(), world);
		if (entity == nullptr)
			return;

		const int_t nearby = (int_t)world->getEntitiesWithinAABB(
			typeid(*entity),
			AxisAlignedBB::getBoundingBoxFromPool(
				(double)sx, (double)sy, (double)sz,
				(double)(sx + 1), (double)(sy + 1), (double)(sz + 1))
				->expand((double)(spawnRange * 2), 4.0, (double)(spawnRange * 2)))
			.size();

		if (nearby >= maxNearbyEntities)
		{
			delete entity;
			resetTimer();
			return;
		}

		const double ex = (double)sx + world->rand.nextDoubleDifference() * (double)spawnRange;
		const double ey = (double)(sy + world->rand.nextInt(3) - 1);
		const double ez = (double)sz + world->rand.nextDoubleDifference() * (double)spawnRange;
		EntityLiving *living = dynamic_cast<EntityLiving *>(entity);
		entity->setLocationAndAngles(ex, ey, ez, world->rand.nextFloat() * 360.0f, 0.0f);

		if (living == nullptr || living->getCanSpawnHere())
		{
			spawnEntityWithData(entity);
			world->playAuxSFX(2004, sx, sy, sz, 0);
			if (living != nullptr)
				living->spawnExplosionParticle();
			spawnedAny = true;
		}
		else
		{
			delete entity;
		}
	}

	if (spawnedAny)
		resetTimer();
}

Entity *MobSpawnerBaseLogic::spawnEntityWithData(Entity *entity)
{
	World *world = getSpawnerWorld();
	if (randomMinecart != nullptr && randomMinecart->properties != nullptr)
	{
		NBTTagCompound baseTag;
		entity->writeToNBT(&baseTag);
		for (NBTBase *tag : randomMinecart->properties->getTags())
		{
			if (tag != nullptr)
				baseTag.setTag(tag->getName(), tag->copy());
		}
		entity->readFromNBT(&baseTag);

		if (world != nullptr && !world->entityJoinedWorld(entity))
		{
			delete entity;
			return nullptr;
		}

		Entity *rider = entity;
		NBTTagCompound *curProps = randomMinecart->properties;
		while (curProps != nullptr && curProps->hasKey("Riding"))
		{
			NBTTagCompound *ridingTag = curProps->getCompoundTag("Riding");
			Entity *mount = EntityList::createEntityInWorld(ridingTag->getString("id"), world);
			if (mount != nullptr)
			{
				NBTTagCompound mountTag;
				mount->writeToNBT(&mountTag);
				for (NBTBase *tag : ridingTag->getTags())
				{
					if (tag != nullptr)
						mountTag.setTag(tag->getName(), tag->copy());
				}
				mount->readFromNBT(&mountTag);
				mount->setLocationAndAngles(rider->posX, rider->posY, rider->posZ, rider->rotationYaw, rider->rotationPitch);
				if (world != nullptr && world->entityJoinedWorld(mount))
				{
					rider->mountEntity(mount);
					rider = mount;
				}
				else
				{
					delete mount;
					break;
				}
			}
			else
			{
				break;
			}
			curProps = ridingTag;
		}
		return entity;
	}

	if (EntityLiving *living = dynamic_cast<EntityLiving *>(entity))
	{
		if (world != nullptr)
		{
			living->initCreature();
			if (!world->entityJoinedWorld(entity))
			{
				delete entity;
				return nullptr;
			}
		}
	}
	else if (world != nullptr)
	{
		if (!world->entityJoinedWorld(entity))
		{
			delete entity;
			return nullptr;
		}
	}
	return entity;
}

void MobSpawnerBaseLogic::resetTimer()
{
	World *world = getSpawnerWorld();
	if (maxSpawnDelay <= minSpawnDelay)
	{
		spawnDelay = minSpawnDelay;
	}
	else if (world != nullptr)
	{
		spawnDelay = minSpawnDelay + world->rand.nextInt(maxSpawnDelay - minSpawnDelay);
	}
	else
	{
		spawnDelay = minSpawnDelay;
	}

	if (!potentialSpawns.empty() && world != nullptr)
	{
		int_t totalWeight = 0;
		for (const WeightedSpawnPotential &p : potentialSpawns)
		{
			if (p.weight > 0)
				totalWeight += p.weight;
		}
		if (totalWeight > 0)
		{
			int_t pick = world->rand.nextInt(totalWeight);
			for (const WeightedSpawnPotential &p : potentialSpawns)
			{
				if (p.weight <= 0)
					continue;
				pick -= p.weight;
				if (pick < 0)
				{
					setRandomMinecart(&p);
					break;
				}
			}
		}
	}

	broadcastEvent(1);
}

void MobSpawnerBaseLogic::readFromNBT(NBTTagCompound *nbt)
{
	mobID = nbt->getString("EntityId");
	if (mobID.empty())
		mobID = "Pig";
	spawnDelay = nbt->getShort("Delay");

	potentialSpawns.clear();
	setRandomMinecart(nullptr);

	if (nbt->hasKey("SpawnPotentials"))
	{
		NBTTagList *list = nbt->getTagList("SpawnPotentials");
		if (list != nullptr)
		{
			for (int_t i = 0; i < list->tagCount(); ++i)
			{
				NBTTagCompound *entryTag = dynamic_cast<NBTTagCompound *>(list->tagAt(i));
				if (entryTag == nullptr)
					continue;
				std::string entryType = entryTag->getString("Type");
				if (entryType == "Minecart")
				{
					if (entryTag->hasKey("Properties"))
					{
						NBTTagCompound *props = entryTag->getCompoundTag("Properties");
						const int_t t = props != nullptr ? props->getInteger("Type") : 0;
						entryType = (t == 1) ? "MinecartChest" : ((t == 2) ? "MinecartFurnace" : "MinecartRideable");
					}
					else
					{
						entryType = "MinecartRideable";
					}
				}
				const int_t entryWeight = entryTag->hasKey("Weight") ? entryTag->getInteger("Weight") : 1;
				NBTTagCompound *entryProps = entryTag->hasKey("Properties") ? entryTag->getCompoundTag("Properties") : nullptr;
				potentialSpawns.emplace_back(entryWeight, entryType, entryProps);
			}
		}
	}

	if (nbt->hasKey("SpawnData"))
	{
		NBTTagCompound *data = nbt->getCompoundTag("SpawnData");
		WeightedSpawnPotential single(1, mobID, data);
		setRandomMinecart(&single);
	}

	if (nbt->hasKey("MinSpawnDelay"))
	{
		minSpawnDelay = nbt->getShort("MinSpawnDelay");
		maxSpawnDelay = nbt->getShort("MaxSpawnDelay");
		spawnCount = nbt->getShort("SpawnCount");
	}
	else
	{
		minSpawnDelay = 200;
		maxSpawnDelay = 800;
		spawnCount = 4;
	}

	if (nbt->hasKey("MaxNearbyEntities"))
	{
		maxNearbyEntities = nbt->getShort("MaxNearbyEntities");
		activatingRangeFromPlayer = nbt->getShort("RequiredPlayerRange");
	}
	else
	{
		maxNearbyEntities = 6;
		activatingRangeFromPlayer = 16;
	}

	if (nbt->hasKey("SpawnRange"))
	{
		spawnRange = nbt->getShort("SpawnRange");
	}
	else
	{
		spawnRange = 4;
	}

	clearCachedEntity();
}

void MobSpawnerBaseLogic::writeToNBT(NBTTagCompound *nbt)
{
	nbt->setString("EntityId", getEntityNameToSpawn());
	nbt->setShort("Delay", (short)spawnDelay);
	nbt->setShort("MinSpawnDelay", (short)minSpawnDelay);
	nbt->setShort("MaxSpawnDelay", (short)maxSpawnDelay);
	nbt->setShort("SpawnCount", (short)spawnCount);
	nbt->setShort("MaxNearbyEntities", (short)maxNearbyEntities);
	nbt->setShort("RequiredPlayerRange", (short)activatingRangeFromPlayer);
	nbt->setShort("SpawnRange", (short)spawnRange);

	if (randomMinecart != nullptr && randomMinecart->properties != nullptr)
	{
		nbt->setTag("SpawnData", randomMinecart->properties->copy());
	}

	if (randomMinecart != nullptr || !potentialSpawns.empty())
	{
		NBTTagList *list = new NBTTagList();
		if (!potentialSpawns.empty())
		{
			for (const WeightedSpawnPotential &p : potentialSpawns)
			{
				NBTTagCompound *entryTag = new NBTTagCompound();
				entryTag->setString("Type", p.type);
				entryTag->setInteger("Weight", p.weight);
				if (p.properties != nullptr)
					entryTag->setTag("Properties", p.properties->copy());
				list->appendTag(entryTag);
			}
		}
		else if (randomMinecart != nullptr)
		{
			NBTTagCompound *entryTag = new NBTTagCompound();
			entryTag->setString("Type", randomMinecart->type);
			entryTag->setInteger("Weight", randomMinecart->weight);
			if (randomMinecart->properties != nullptr)
				entryTag->setTag("Properties", randomMinecart->properties->copy());
			list->appendTag(entryTag);
		}
		nbt->setTag("SpawnPotentials", list);
	}
}

Entity *MobSpawnerBaseLogic::getCachedEntity()
{
	if (cachedEntity == nullptr)
	{
		Entity *entity = EntityList::createEntityInWorld(getEntityNameToSpawn(), nullptr);
		if (entity != nullptr)
		{
			if (randomMinecart != nullptr && randomMinecart->properties != nullptr)
			{
				NBTTagCompound baseTag;
				entity->writeToNBT(&baseTag);
				for (NBTBase *tag : randomMinecart->properties->getTags())
				{
					if (tag != nullptr)
						baseTag.setTag(tag->getName(), tag->copy());
				}
				entity->readFromNBT(&baseTag);
			}
			cachedEntity = entity;
		}
	}
	return cachedEntity;
}

bool MobSpawnerBaseLogic::setDelayToMin(int_t eventId)
{
	World *world = getSpawnerWorld();
	if (eventId == 1 && world != nullptr && world->multiplayerWorld)
	{
		spawnDelay = minSpawnDelay;
		return true;
	}
	return false;
}
