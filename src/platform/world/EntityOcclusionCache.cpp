#include "platform/world/EntityOcclusionCache.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

#include "net/minecraft/src/Block.h"
#include "net/minecraft/src/Entity.h"
#include "net/minecraft/src/MathHelper.h"
#include "net/minecraft/src/World.h"
#include "java/Arithmetic.h"
#include "platform/PlatformTuning.h"

namespace
{
constexpr double kMinCullDistanceSq = 6.25;
constexpr double kMaxCullAabbSize = 4.0;
// Only hide things well below the camera (mobs and chests in caves while the
// player is on the surface): hiding a cow behind a hill made it pop in and out.
constexpr double kMinBelowCameraBlocks = 6.0;
constexpr std::size_t kMinEntriesBeforePurge = 128;
constexpr std::uint32_t kPurgeInterval = 600;

bool rayBlockedByOpaqueBlocks(World *world, double ox, double oy, double oz,
                              double tx, double ty, double tz)
{
	const double deltaX = tx - ox;
	const double deltaY = ty - oy;
	const double deltaZ = tz - oz;
	const double distanceSq = deltaX * deltaX + deltaY * deltaY + deltaZ * deltaZ;
	if (distanceSq <= 0.0)
		return false;

	const double distance = std::sqrt(distanceSq);

	const int startX = MathHelper::floor_double(ox);
	const int startY = MathHelper::floor_double(oy);
	const int startZ = MathHelper::floor_double(oz);
	const int targetX = MathHelper::floor_double(tx);
	const int targetY = MathHelper::floor_double(ty);
	const int targetZ = MathHelper::floor_double(tz);

	if (startX == targetX && startY == targetY && startZ == targetZ)
		return false;

	int lastChunkX = std::numeric_limits<int>::max();
	int lastChunkZ = std::numeric_limits<int>::max();
	const auto cellInLoadedChunk = [&](int blockX, int blockZ) -> bool
	{
		const int chunkX = JavaArithmetic::intShr(blockX, 4);
		const int chunkZ = JavaArithmetic::intShr(blockZ, 4);
		if (chunkX == lastChunkX && chunkZ == lastChunkZ)
			return true;
		if (!world->chunkExists(chunkX, chunkZ))
			return false;
		lastChunkX = chunkX;
		lastChunkZ = chunkZ;
		return true;
	};

	if (!cellInLoadedChunk(startX, startZ))
		return false;
	{
		const int_t startBlockId = world->getBlockId(startX, startY, startZ);
		if (startBlockId > 0 && startBlockId < Block::BLOCK_REGISTRY_SIZE
			&& Block::opaqueCubeLookup[startBlockId])
		{
			return false;
		}
	}

	const double dirX = deltaX / distance;
	const double dirY = deltaY / distance;
	const double dirZ = deltaZ / distance;

	const int stepX = dirX > 0.0 ? 1 : (dirX < 0.0 ? -1 : 0);
	const int stepY = dirY > 0.0 ? 1 : (dirY < 0.0 ? -1 : 0);
	const int stepZ = dirZ > 0.0 ? 1 : (dirZ < 0.0 ? -1 : 0);

	// Use a large finite sentinel instead of infinity to avoid any x87 invalid-op edge cases.
	constexpr double kNever = 1.0e30;
	const double tDeltaX = stepX != 0 ? std::fabs(1.0 / dirX) : kNever;
	const double tDeltaY = stepY != 0 ? std::fabs(1.0 / dirY) : kNever;
	const double tDeltaZ = stepZ != 0 ? std::fabs(1.0 / dirZ) : kNever;

	double tMaxX = stepX != 0
		? (stepX > 0 ? (startX + 1 - ox) : (ox - startX)) * tDeltaX
		: kNever;
	double tMaxY = stepY != 0
		? (stepY > 0 ? (startY + 1 - oy) : (oy - startY)) * tDeltaY
		: kNever;
	double tMaxZ = stepZ != 0
		? (stepZ > 0 ? (startZ + 1 - oz) : (oz - startZ)) * tDeltaZ
		: kNever;

	int cellX = startX;
	int cellY = startY;
	int cellZ = startZ;

	int stepsLeft = static_cast<int>(distance * 3.0) + 8;

	while (stepsLeft-- > 0)
	{
		if (tMaxX <= tMaxY && tMaxX <= tMaxZ)
		{
			if (tMaxX > distance)
				return false;
			cellX += stepX;
			tMaxX += tDeltaX;
		}
		else if (tMaxY <= tMaxZ)
		{
			if (tMaxY > distance)
				return false;
			cellY += stepY;
			tMaxY += tDeltaY;
		}
		else
		{
			if (tMaxZ > distance)
				return false;
			cellZ += stepZ;
			tMaxZ += tDeltaZ;
		}

		if (cellX == targetX && cellY == targetY && cellZ == targetZ)
			return false;

		if (!cellInLoadedChunk(cellX, cellZ))
			return false;

		const int_t blockId = world->getBlockId(cellX, cellY, cellZ);
		if (blockId > 0 && blockId < Block::BLOCK_REGISTRY_SIZE
			&& Block::opaqueCubeLookup[blockId])
		{
			return true;
		}
	}

	return false;
}
} // namespace

EntityOcclusionCache::Entry *EntityOcclusionCache::findEntry(std::uint64_t key)
{
	for (Entry &entry : entries)
	{
		if (entry.key == key)
			return &entry;
	}
	return nullptr;
}

const EntityOcclusionCache::Entry *EntityOcclusionCache::findEntry(std::uint64_t key) const
{
	for (const Entry &entry : entries)
	{
		if (entry.key == key)
			return &entry;
	}
	return nullptr;
}

std::uint64_t EntityOcclusionCache::tileEntityKey(int_t blockX, int_t blockY, int_t blockZ)
{
	// Pack 26-bit X, 26-bit Z, 8-bit Y (supports 0..255 height in 1.5.2) + top tag bit.
	constexpr std::uint64_t kTileEntityTag = 0x8000000000000000ULL;
	const std::uint64_t packedX =
		(static_cast<std::uint64_t>(static_cast<std::int64_t>(blockX) + (1LL << 25)) & 0x3FFFFFFULL) << 34;
	const std::uint64_t packedZ =
		(static_cast<std::uint64_t>(static_cast<std::int64_t>(blockZ) + (1LL << 25)) & 0x3FFFFFFULL) << 8;
	const std::uint64_t packedY = static_cast<std::uint64_t>(blockY) & 0xFFULL;
	return kTileEntityTag | packedX | packedZ | packedY;
}

void EntityOcclusionCache::beginFrame()
{
	++frame;
	raycastsLeft = PLATFORM_ENTITY_OCCLUSION_RAYCASTS_PER_FRAME;

	if (entries.size() > kMinEntriesBeforePurge && (frame % kPurgeInterval) == 0)
	{
		entries.erase(std::remove_if(entries.begin(), entries.end(),
			[this](const Entry &entry)
			{
				return (frame - entry.lastSeenFrame) >= kPurgeInterval;
			}), entries.end());
	}
}

bool EntityOcclusionCache::isEntityOccluded(World *world, Entity *entity,
                                            double cameraX, double cameraY, double cameraZ)
{
	if (world == nullptr || entity == nullptr || entity->boundingBox == nullptr)
		return false;

	if (entity->riddenByEntity != nullptr)
		return false;

	const AxisAlignedBB &box = *entity->boundingBox;
	const double sizeX = box.maxX - box.minX;
	const double sizeY = box.maxY - box.minY;
	const double sizeZ = box.maxZ - box.minZ;
	if (sizeX > kMaxCullAabbSize || sizeY > kMaxCullAabbSize || sizeZ > kMaxCullAabbSize)
		return false;

	const double centerX = (box.minX + box.maxX) * 0.5;
	const double centerY = (box.minY + box.maxY) * 0.5;
	const double centerZ = (box.minZ + box.maxZ) * 0.5;
	const double dx = centerX - cameraX;
	const double dy = centerY - cameraY;
	const double dz = centerZ - cameraZ;
	const double distanceSq = dx * dx + dy * dy + dz * dz;
	if (distanceSq < kMinCullDistanceSq)
		return false;
	if (dy > -kMinBelowCameraBlocks)
		return false;

	return occludedWithCache(world, static_cast<std::uint64_t>(
		static_cast<std::int64_t>(entity->entityId)),
		cameraX, cameraY, cameraZ, centerX, centerY, centerZ);
}

bool EntityOcclusionCache::isTileEntityOccluded(World *world, int_t blockX, int_t blockY, int_t blockZ,
                                                double cameraX, double cameraY, double cameraZ)
{
	if (world == nullptr)
		return false;

	const double centerX = blockX + 0.5;
	const double centerY = blockY + 0.5;
	const double centerZ = blockZ + 0.5;
	const double dx = centerX - cameraX;
	const double dy = centerY - cameraY;
	const double dz = centerZ - cameraZ;
	const double distanceSq = dx * dx + dy * dy + dz * dz;
	if (distanceSq < kMinCullDistanceSq)
		return false;
	if (dy > -kMinBelowCameraBlocks)
		return false;

	return occludedWithCache(world, tileEntityKey(blockX, blockY, blockZ),
	                         cameraX, cameraY, cameraZ, centerX, centerY, centerZ);
}

bool EntityOcclusionCache::occludedWithCache(World *world, std::uint64_t key,
                                             double cameraX, double cameraY, double cameraZ,
                                             double targetX, double targetY, double targetZ)
{
	Entry *entry = findEntry(key);
	if (entry != nullptr)
		entry->lastSeenFrame = frame;

	const bool fresh = entry != nullptr
		&& (frame - entry->lastRayFrame) < static_cast<std::uint32_t>(PLATFORM_ENTITY_OCCLUSION_RECHECK_FRAMES);
	if (fresh)
		return entry->occluded;

	if (raycastsLeft <= 0)
		return entry != nullptr ? entry->occluded : false;

	--raycastsLeft;
	const bool occluded = rayBlockedByOpaqueBlocks(world, cameraX, cameraY, cameraZ,
	                                              targetX, targetY, targetZ);
	if (entry == nullptr)
	{
		Entry newEntry;
		newEntry.key = key;
		newEntry.occluded = occluded;
		newEntry.lastRayFrame = frame;
		newEntry.lastSeenFrame = frame;
		entries.push_back(newEntry);
	}
	else
	{
		entry->occluded = occluded;
		entry->lastRayFrame = frame;
		entry->lastSeenFrame = frame;
	}
	return occluded;
}

void EntityOcclusionCache::clear()
{
	entries.clear();
	raycastsLeft = 0;
}
