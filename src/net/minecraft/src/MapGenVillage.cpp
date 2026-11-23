#include "MapGenVillage.h"

#include "BiomeGenBase.h"
#include "StructureVillageStart.h"
#include "World.h"
#include "WorldChunkManager.h"
#include "java/Random.h"
#include "java/Arithmetic.h"

#include <cstdlib>

std::vector<BiomeGenBase *> MapGenVillage::villageSpawnBiomes;

namespace
{
int_t parseIntDefaultMin(const std::string &s, int_t def, int_t minVal)
{
	if (s.empty())
		return def;
	char *endPtr = nullptr;
	const long v = std::strtol(s.c_str(), &endPtr, 10);
	if (endPtr == s.c_str() || *endPtr != '\0')
		return def;
	const int_t parsed = static_cast<int_t>(v);
	return parsed < minVal ? minVal : parsed;
}
} // namespace

MapGenVillage::MapGenVillage(int_t terrainTypeValue)
	: terrainType(terrainTypeValue), maxDistanceBetweenVillages(32), minDistanceBetweenVillages(8)
{
	if (villageSpawnBiomes.empty())
	{
		villageSpawnBiomes.push_back(BiomeGenBase::plains);
		villageSpawnBiomes.push_back(BiomeGenBase::desert);
	}
}

MapGenVillage::MapGenVillage(const std::map<std::string, std::string> &options)
	: MapGenVillage(0)
{
	for (const auto &entry : options)
	{
		if (entry.first == "size")
			terrainType = parseIntDefaultMin(entry.second, terrainType, 0);
		else if (entry.first == "distance")
			maxDistanceBetweenVillages = parseIntDefaultMin(entry.second, maxDistanceBetweenVillages, minDistanceBetweenVillages + 1);
	}
}

bool MapGenVillage::canSpawnStructureAtCoords(int_t chunkX, int_t chunkZ)
{
	const int_t spacing = maxDistanceBetweenVillages;
	const int_t separation = minDistanceBetweenVillages;
	const int_t originalX = chunkX;
	const int_t originalZ = chunkZ;
	if (chunkX < 0)
		chunkX = JavaArithmetic::intSub(chunkX, spacing - 1);
	if (chunkZ < 0)
		chunkZ = JavaArithmetic::intSub(chunkZ, spacing - 1);

	int_t regionX = chunkX / spacing;
	int_t regionZ = chunkZ / spacing;
	Random &random = worldObj->setRandomSeed(regionX, regionZ, 10387312);
	regionX = JavaArithmetic::intMul(regionX, spacing);
	regionZ = JavaArithmetic::intMul(regionZ, spacing);
	regionX = JavaArithmetic::intAdd(regionX, random.nextInt(spacing - separation));
	regionZ = JavaArithmetic::intAdd(regionZ, random.nextInt(spacing - separation));
	if (originalX != regionX || originalZ != regionZ)
		return false;

	return worldObj->getWorldChunkManager()->areBiomesViable(
		JavaArithmetic::intAdd(JavaArithmetic::intMul(originalX, 16), 8),
		JavaArithmetic::intAdd(JavaArithmetic::intMul(originalZ, 16), 8), 0, villageSpawnBiomes);
}

StructureStart *MapGenVillage::getStructureStart(int_t chunkX, int_t chunkZ)
{
	return new StructureVillageStart(worldObj, rand, chunkX, chunkZ, terrainType);
}
