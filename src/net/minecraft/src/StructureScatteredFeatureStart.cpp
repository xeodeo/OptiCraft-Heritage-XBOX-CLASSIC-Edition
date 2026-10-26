#include "StructureScatteredFeatureStart.h"

#include "BiomeGenBase.h"
#include "ComponentScatteredFeatureDesertPyramid.h"
#include "ComponentScatteredFeatureJunglePyramid.h"
#include "ComponentScatteredFeatureSwampHut.h"
#include "World.h"
#include "java/Random.h"

StructureScatteredFeatureStart::StructureScatteredFeatureStart(World *world, Random &random, int_t chunkX, int_t chunkZ)
{
	BiomeGenBase *biome = world != nullptr ? world->getBiomeGenForCoords(chunkX * 16 + 8, chunkZ * 16 + 8) : nullptr;
	if (biome == BiomeGenBase::jungle || biome == BiomeGenBase::jungleHills)
	{
		addComponent(new ComponentScatteredFeatureJunglePyramid(random, chunkX * 16, chunkZ * 16));
	}
	else if (biome == BiomeGenBase::swampland)
	{
		addComponent(new ComponentScatteredFeatureSwampHut(random, chunkX * 16, chunkZ * 16));
	}
	else
	{
		addComponent(new ComponentScatteredFeatureDesertPyramid(random, chunkX * 16, chunkZ * 16));
	}

	updateBoundingBox();
}
