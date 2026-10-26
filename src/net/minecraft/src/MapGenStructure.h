#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include "MapGenBase.h"
#include "StructureBoundingBox.h"
#include "java/Type.h"
#include "java/HashSet.h"

class ChunkPosition;
class StructureStart;
class World;

// net.minecraft.src.MapGenStructure
class MapGenStructure : public MapGenBase
{
public:
	MapGenStructure();
	~MapGenStructure() override;

	bool generateStructuresInChunk(World *world, Random &random, int_t chunkX, int_t chunkZ);
	bool isInsideStructure(int_t x, int_t y, int_t z) const;
	bool hasStructureAt(int_t x, int_t y, int_t z) const { return isInsideStructure(x, y, z); }
	void appendIntersectingComponentBounds(const StructureBoundingBox &area,
	                                      std::vector<StructureBoundingBox> &out,
	                                      int_t horizontalPadding = 0) const;
	ChunkPosition *getNearestInstance(World *world, int_t x, int_t y, int_t z);

	// Drops every cached StructureStart whose bounding box lies entirely outside
	// the given block range. A start is a pure function of the seed and its
	// source chunk, so the next source sweep that reaches that chunk rebuilds it.
	// No-op while generateStructuresInChunk() is on the stack, because that loop
	// holds raw pointers into coordMap (see the snapshot comment there).
	void trimStructureStarts(int_t minBlockX, int_t minBlockZ, int_t maxBlockX, int_t maxBlockZ);

protected:
	void generateChunk(World *world, int_t sourceChunkX, int_t sourceChunkZ,
	                   int_t targetChunkX, int_t targetChunkZ, byte_t blocks[]) override;
	bool sourceNeedsGeneration(int_t sourceChunkX, int_t sourceChunkZ) override;
	virtual std::vector<ChunkPosition *> getStructureCoordinates();
	virtual bool projectDecorationReservationToAreaHeight() const { return false; }
	virtual bool canSpawnStructureAtCoords(int_t chunkX, int_t chunkZ) = 0;
	virtual StructureStart *getStructureStart(int_t chunkX, int_t chunkZ) = 0;

	struct StructureKeyHash
	{
		std::uint32_t operator()(ulong_t value) const
		{
			// java.lang.Long.hashCode(): (int)(value ^ (value >>> 32)).
			return static_cast<std::uint32_t>(value ^ (value >> 32));
		}
	};

	struct StructureKeyEqual
	{
		bool operator()(ulong_t lhs, ulong_t rhs) const { return lhs == rhs; }
	};

	std::unordered_map<ulong_t, StructureStart *> coordMap;
	JavaHashSet<ulong_t, StructureKeyHash, StructureKeyEqual> coordOrder;

private:
	static constexpr std::size_t NEGATIVE_SOURCE_CACHE_SIZE = 1024;
	static constexpr std::size_t NEGATIVE_SOURCE_CACHE_WAYS = 4;
	static constexpr std::size_t NEGATIVE_SOURCE_CACHE_SET_COUNT =
		NEGATIVE_SOURCE_CACHE_SIZE / NEGATIVE_SOURCE_CACHE_WAYS;
	static constexpr std::size_t NEGATIVE_SOURCE_CACHE_VALID_WORDS = NEGATIVE_SOURCE_CACHE_SIZE / 32;

	std::size_t negativeSourceCacheSet(ulong_t key) const;
	bool isNegativeSourceCached(ulong_t key) const;
	void cacheNegativeSource(ulong_t key);

	std::array<ulong_t, NEGATIVE_SOURCE_CACHE_SIZE> negativeSourceCache{};
	std::array<std::uint32_t, NEGATIVE_SOURCE_CACHE_VALID_WORDS> negativeSourceCacheValid{};
	std::array<std::uint8_t, NEGATIVE_SOURCE_CACHE_SET_COUNT> negativeSourceCacheNextWay{};

	// Depth of generateStructuresInChunk() calls in progress; guards trimStructureStarts().
	int_t structureIterationDepth = 0;
};
