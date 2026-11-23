#pragma once

#include <map>
#include <string>
#include <vector>

#include "MapGenStructure.h"

class BiomeGenBase;

// net.minecraft.src.MapGenVillage
class MapGenVillage : public MapGenStructure
{
public:
	explicit MapGenVillage(int_t terrainType);
	explicit MapGenVillage(const std::map<std::string, std::string> &options);

	static std::vector<BiomeGenBase *> villageSpawnBiomes;

protected:
	bool projectDecorationReservationToAreaHeight() const override { return true; }
	bool canSpawnStructureAtCoords(int_t chunkX, int_t chunkZ) override;
	StructureStart *getStructureStart(int_t chunkX, int_t chunkZ) override;

private:
	int_t terrainType;
	int_t maxDistanceBetweenVillages;
	int_t minDistanceBetweenVillages;
};
