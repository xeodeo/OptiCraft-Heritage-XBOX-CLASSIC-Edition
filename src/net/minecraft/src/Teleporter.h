#pragma once

#include "java/Random.h"
#include "java/Type.h"

#include <map>
#include <utility>

class Entity;
class ISaveHandler;
class World;

struct PortalPosition
{
	int_t posX;
	int_t posY;
	int_t posZ;
	long_t lastUpdateTime;
};

// net.minecraft.src.Teleporter
class Teleporter
{
public:
	Teleporter();

	void placeInPortal(World *world, Entity *entity);
	bool placeInExistingPortal(World *world, Entity *entity);
	bool createPortal(World *world, Entity *entity);
	void removeStalePortalLocations(long_t worldTime);
	void clearDestinationCoordinateCache();
	void tick(long_t totalWorldTime);

private:
	Random field_4232_a;
	ISaveHandler *cacheSaveHandler = nullptr;
	long_t currentWorldTime = 0;
	std::map<std::pair<int_t, long_t>, PortalPosition> destinationCoordinateCache;
};
