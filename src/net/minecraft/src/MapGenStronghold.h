#pragma once

#include <map>
#include <string>
#include <vector>

#include "MapGenStructure.h"

class BiomeGenBase;
class ChunkCoordIntPair;
class ChunkPosition;
class StructureStart;

// net.minecraft.src.MapGenStronghold
class MapGenStronghold : public MapGenStructure
{
public:
	MapGenStronghold();
	explicit MapGenStronghold(const std::map<std::string, std::string> &options);
	~MapGenStronghold() override;

protected:
	bool canSpawnStructureAtCoords(int_t chunkX, int_t chunkZ) override;
	StructureStart *getStructureStart(int_t chunkX, int_t chunkZ) override;
	std::vector<ChunkPosition *> getStructureCoordinates() override;

private:
	void ensureStructureCoords();

	std::vector<BiomeGenBase *> allowedBiomeGenBases;
	bool ranBiomeCheck;
	std::vector<ChunkCoordIntPair *> structureCoords;
	double strongholdDistance;
	int_t strongholdSpread;
	bool useCustomParams;
};
