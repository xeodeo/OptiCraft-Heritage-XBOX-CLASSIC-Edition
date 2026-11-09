#pragma once

#include <cstddef>
#include <cstdint>
#include <set>
#include <unordered_map>
#include <vector>

#include "java/HashSet.h"
#include "java/Type.h"
#include "net/minecraft/src/NextTickListEntry.h"

class PcLegacyTickScheduler
{
public:
	PcLegacyTickScheduler() = default;
	~PcLegacyTickScheduler();

	PcLegacyTickScheduler(const PcLegacyTickScheduler &) = delete;
	PcLegacyTickScheduler &operator=(const PcLegacyTickScheduler &) = delete;

	bool schedule(int_t x, int_t y, int_t z, int_t blockId, long_t scheduledTime, int_t priority = 0);
	bool contains(int_t x, int_t y, int_t z, int_t blockId) const;
	NextTickListEntry *popNext(long_t currentTime, bool force);
	std::vector<NextTickListEntry *> getPendingForChunk(int_t chunkX, int_t chunkZ, bool remove);

	void clear();
	std::size_t size() const;
	bool empty() const;

private:
	using ChunkKey = std::uint64_t;
	using ChunkEntries = std::vector<NextTickListEntry *>;

	static ChunkKey makeChunkKey(int_t chunkX, int_t chunkZ);
	static ChunkKey makeChunkKeyForEntry(const NextTickListEntry *entry);

	void attachToChunk(NextTickListEntry *entry);
	void detachFromChunk(NextTickListEntry *entry);
	void detachFromCoreIndexes(NextTickListEntry *entry);

	std::set<NextTickListEntry *, NextTickListEntryComparator> orderedEntries;
	JavaHashSet<NextTickListEntry *, NextTickListEntryHash, NextTickListEntryEqual> uniqueEntries;
	std::unordered_map<ChunkKey, ChunkEntries> entriesByChunk;
};
