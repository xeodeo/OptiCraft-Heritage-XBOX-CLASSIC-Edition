#pragma once

#include <string>
#include "WorldSavedData.h"

class MapDataBase;

using MapDataFactory = MapDataBase *(*)(const std::string &name);

// net.minecraft.src.MapDataBase; all map and village saved data use the
// same persistence and dirty tracking contract.
class MapDataBase : public WorldSavedData
{
public:
	explicit MapDataBase(const std::string &name);
	~MapDataBase() override = default;
};