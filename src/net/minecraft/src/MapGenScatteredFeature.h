#pragma once

#include "MapGenStructure.h"
#include "SpawnListEntry.h"
#include <map>
#include <string>
#include <vector>

// net.minecraft.src.MapGenScatteredFeature
class MapGenScatteredFeature : public MapGenStructure
{
public:
	MapGenScatteredFeature();
	explicit MapGenScatteredFeature(const std::map<std::string, std::string> &options);
	~MapGenScatteredFeature() override = default;

	std::vector<SpawnListEntry> &getScatteredFeatureSpawnList();

protected:
	bool canSpawnStructureAtCoords(int_t chunkX, int_t chunkZ) override;
	StructureStart *getStructureStart(int_t chunkX, int_t chunkZ) override;

private:
	std::vector<SpawnListEntry> scatteredFeatureSpawnList;
	int_t maxDistanceBetweenScatteredFeatures;
	int_t minDistanceBetweenScatteredFeatures;
};
