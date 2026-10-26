#include "MapGenScatteredFeature.h"

#include "BiomeGenBase.h"
#include "EntityWitch.h"
#include "StructureScatteredFeatureStart.h"
#include "World.h"
#include "WorldChunkManager.h"
#include "java/Random.h"

MapGenScatteredFeature::MapGenScatteredFeature()
	: maxDistanceBetweenScatteredFeatures(32),
	  minDistanceBetweenScatteredFeatures(8)
{
	scatteredFeatureSpawnList.push_back(SpawnListEntry(typeid(EntityWitch), 1, 1, 1));
}

std::vector<SpawnListEntry> &MapGenScatteredFeature::getScatteredFeatureSpawnList()
{
	return scatteredFeatureSpawnList;
}

bool MapGenScatteredFeature::canSpawnStructureAtCoords(int_t chunkX, int_t chunkZ)
{
	int_t origX = chunkX;
	int_t origZ = chunkZ;

	if (chunkX < 0)
		chunkX -= maxDistanceBetweenScatteredFeatures - 1;
	if (chunkZ < 0)
		chunkZ -= maxDistanceBetweenScatteredFeatures - 1;

	int_t gridX = chunkX / maxDistanceBetweenScatteredFeatures;
	int_t gridZ = chunkZ / maxDistanceBetweenScatteredFeatures;

	Random &random = worldObj->setRandomSeed(gridX, gridZ, 14357617);
	gridX *= maxDistanceBetweenScatteredFeatures;
	gridZ *= maxDistanceBetweenScatteredFeatures;
	gridX += random.nextInt(maxDistanceBetweenScatteredFeatures - minDistanceBetweenScatteredFeatures);
	gridZ += random.nextInt(maxDistanceBetweenScatteredFeatures - minDistanceBetweenScatteredFeatures);

	if (origX == gridX && origZ == gridZ)
	{
		BiomeGenBase *biome = worldObj->getWorldChunkManager()->getBiomeGenAt(origX * 16 + 8, origZ * 16 + 8);
		if (biome == BiomeGenBase::desert || biome == BiomeGenBase::desertHills ||
		    biome == BiomeGenBase::jungle || biome == BiomeGenBase::jungleHills ||
		    biome == BiomeGenBase::swampland)
		{
			return true;
		}
	}

	return false;
}

StructureStart *MapGenScatteredFeature::getStructureStart(int_t chunkX, int_t chunkZ)
{
	return new StructureScatteredFeatureStart(worldObj, rand, chunkX, chunkZ);
}
