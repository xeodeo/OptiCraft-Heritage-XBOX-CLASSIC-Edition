#include "WorldSettings.h"

WorldSettings::WorldSettings(int64_t worldSeed, int_t type, bool featuresEnabled,
                             bool hardcore, WorldType *worldType, int_t sizeType) :
	seed(worldSeed), gameType(type), mapFeaturesEnabled(featuresEnabled),
	hardcoreEnabled(hardcore), terrainType(worldType), worldSizeType(sizeType)
{
}

int64_t WorldSettings::getSeed() const
{
	return seed;
}

int_t WorldSettings::getGameType() const
{
	return gameType;
}

bool WorldSettings::getHardcoreEnabled() const
{
	return hardcoreEnabled;
}

bool WorldSettings::isMapFeaturesEnabled() const
{
	return mapFeaturesEnabled;
}

WorldType *WorldSettings::getTerrainType() const
{
	return terrainType;
}

bool WorldSettings::isLimitedWorld() const
{
	return worldSizeType != 0;
}

int_t WorldSettings::getWorldSizeType() const
{
	return worldSizeType;
}



