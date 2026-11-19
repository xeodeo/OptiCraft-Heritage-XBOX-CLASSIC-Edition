#pragma once

#include <cstdint>
#include <vector>

#include "java/Type.h"

class World;
class Entity;

// EntityCulling-style occlusion culling for the entity and tile-entity render
// passes: casts a budgeted voxel DDA ray from the camera to the target and
// caches the visibility answer for a few frames.
class EntityOcclusionCache
{
public:
	void beginFrame();

	bool isEntityOccluded(World *world, Entity *entity,
	                      double cameraX, double cameraY, double cameraZ);

	bool isTileEntityOccluded(World *world, int_t blockX, int_t blockY, int_t blockZ,
	                          double cameraX, double cameraY, double cameraZ);

	void clear();

private:
	struct Entry
	{
		std::uint64_t key = 0;
		bool occluded = false;
		std::uint32_t lastRayFrame = 0;
		std::uint32_t lastSeenFrame = 0;
	};

	bool occludedWithCache(World *world, std::uint64_t key,
	                       double cameraX, double cameraY, double cameraZ,
	                       double targetX, double targetY, double targetZ);

	static std::uint64_t tileEntityKey(int_t blockX, int_t blockY, int_t blockZ);

	Entry *findEntry(std::uint64_t key);
	const Entry *findEntry(std::uint64_t key) const;

	std::vector<Entry> entries;
	std::uint32_t frame = 0;
	int raycastsLeft = 0;
};
