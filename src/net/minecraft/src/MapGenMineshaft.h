#pragma once

#include <map>
#include <string>

#include "MapGenStructure.h"

// net.minecraft.src.MapGenMineshaft
class MapGenMineshaft : public MapGenStructure
{
public:
	MapGenMineshaft() = default;
	explicit MapGenMineshaft(const std::map<std::string, std::string> &options);

protected:
	bool canSpawnStructureAtCoords(int_t chunkX, int_t chunkZ) override;
	StructureStart *getStructureStart(int_t chunkX, int_t chunkZ) override;

private:
	double field_82673_e = 0.01;
	bool useCustomChance = false;
};
