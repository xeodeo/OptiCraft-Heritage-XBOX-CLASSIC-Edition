#pragma once

#include "java/Type.h"

class WorldType;

// net.minecraft.src.WorldSettings
class WorldSettings
{
public:
	WorldSettings(int64_t seed, int_t gameType, bool mapFeaturesEnabled,
	              bool hardcoreEnabled, WorldType *terrainType, int_t worldSizeType = 0);

	int64_t getSeed() const;
	int_t getGameType() const;
	bool getHardcoreEnabled() const;
	bool isMapFeaturesEnabled() const;
	WorldType *getTerrainType() const;
	bool isLimitedWorld() const;
	int_t getWorldSizeType() const;

private:
	int64_t seed;
	int_t gameType;
	bool mapFeaturesEnabled;
	bool hardcoreEnabled;
	WorldType *terrainType;
	int_t worldSizeType;
};

