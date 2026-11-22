#pragma once

#include <vector>

#include "MapGenStructure.h"

class BiomeGenBase;
class ChunkPosition;

// net.minecraft.src.MapGenVillage
class MapGenVillage : public MapGenStructure
{
public:
	explicit MapGenVillage(int_t terrainType);

	ChunkPosition *findNearestVillage(World *world, int_t x, int_t y, int_t z, int_t maxRegionRadius);

	static std::vector<BiomeGenBase *> villageSpawnBiomes;

protected:
	bool projectDecorationReservationToAreaHeight() const override { return true; }
	bool canSpawnStructureAtCoords(int_t chunkX, int_t chunkZ) override;
	StructureStart *getStructureStart(int_t chunkX, int_t chunkZ) override;

private:
	const int_t terrainType;
};
