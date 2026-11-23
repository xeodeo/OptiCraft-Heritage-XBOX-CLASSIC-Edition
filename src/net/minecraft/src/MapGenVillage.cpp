#include "MapGenVillage.h"

#include "BiomeGenBase.h"
#include "ChunkPosition.h"
#include "StructureComponent.h"
#include "StructureVillageStart.h"
#include "World.h"
#include "WorldChunkManager.h"
#include "java/Random.h"
#include "java/Arithmetic.h"

#include <cstdlib>
#include <memory>
#include <vector>

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

ChunkPosition *MapGenVillage::findNearestVillage(World *world, int_t x, int_t y, int_t z, int_t maxRegionRadius)
{
	(void)y;
	if (world == nullptr)
		return nullptr;
	WorldChunkManager *manager = world->getWorldChunkManager();
	if (manager == nullptr)
		return nullptr;

	// Use exact vanilla 32/8 spacing & separation so terrain/village locations are 100% unchanged.
	static constexpr int_t SPACING = 32;
	static constexpr int_t SEPARATION = 8;

	worldObj = world;
	const long_t worldSeed = prepareSourceSeeding(world);

	const int_t playerChunkX = JavaArithmetic::intShr(x, 4);
	const int_t playerChunkZ = JavaArithmetic::intShr(z, 4);
	const int_t playerRegionX = playerChunkX >= 0
		? playerChunkX / SPACING
		: (playerChunkX - (SPACING - 1)) / SPACING;
	const int_t playerRegionZ = playerChunkZ >= 0
		? playerChunkZ / SPACING
		: (playerChunkZ - (SPACING - 1)) / SPACING;

	const int_t radius = maxRegionRadius < 0 ? 0 : (maxRegionRadius > 16 ? 16 : maxRegionRadius);

	struct Candidate
	{
		int_t chunkX;
		int_t chunkZ;
		double distanceSq;
	};
	std::vector<Candidate> candidates;
	candidates.reserve(static_cast<std::size_t>((radius * 2 + 1) * (radius * 2 + 1)));

	for (int_t regionX = playerRegionX - radius; regionX <= playerRegionX + radius; ++regionX)
	{
		for (int_t regionZ = playerRegionZ - radius; regionZ <= playerRegionZ + radius; ++regionZ)
		{
			const ulong_t seedBits =
				static_cast<ulong_t>(static_cast<long_t>(regionX)) * 341873128712ULL +
				static_cast<ulong_t>(static_cast<long_t>(regionZ)) * 132897987541ULL +
				static_cast<ulong_t>(worldSeed) + 10387312ULL;
			Random regionRandom(JavaArithmetic::longFromBits(seedBits));
			const int_t candidateChunkX = JavaArithmetic::intAdd(
				JavaArithmetic::intMul(regionX, SPACING),
				regionRandom.nextInt(SPACING - SEPARATION));
			const int_t candidateChunkZ = JavaArithmetic::intAdd(
				JavaArithmetic::intMul(regionZ, SPACING),
				regionRandom.nextInt(SPACING - SEPARATION));

			const int_t blockX = JavaArithmetic::intAdd(JavaArithmetic::intMul(candidateChunkX, 16), 8);
			const int_t blockZ = JavaArithmetic::intAdd(JavaArithmetic::intMul(candidateChunkZ, 16), 8);
			if (!manager->areBiomesViable(blockX, blockZ, 0, villageSpawnBiomes))
				continue;

			const double dx = static_cast<double>(blockX) - static_cast<double>(x);
			const double dz = static_cast<double>(blockZ) - static_cast<double>(z);
			candidates.push_back({candidateChunkX, candidateChunkZ, dx * dx + dz * dz});
		}
	}

	for (std::size_t i = 1; i < candidates.size(); ++i)
	{
		const Candidate key = candidates[i];
		std::size_t j = i;
		while (j > 0 && candidates[j - 1].distanceSq > key.distanceSq)
		{
			candidates[j] = candidates[j - 1];
			--j;
		}
		candidates[j] = key;
	}

	for (const Candidate &candidate : candidates)
	{
		rand.setSeed(sourceSeed(candidate.chunkX, candidate.chunkZ, worldSeed));
		rand.nextInt();
		std::unique_ptr<StructureStart> start(getStructureStart(candidate.chunkX, candidate.chunkZ));
		if (!start || !start->isSizeableStructure() || start->getComponents().empty())
			continue;

		ChunkPosition *center = start->getComponents().front()->getCenter();
		if (center != nullptr)
			return center;
	}

	return nullptr;
}

StructureStart *MapGenVillage::getStructureStart(int_t chunkX, int_t chunkZ)
{
	return new StructureVillageStart(worldObj, rand, chunkX, chunkZ, terrainType);
}
