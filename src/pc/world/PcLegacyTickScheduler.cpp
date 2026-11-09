#include "pc/world/PcLegacyTickScheduler.h"

#include <algorithm>
#include <cstdint>

#include "java/Arithmetic.h"

PcLegacyTickScheduler::~PcLegacyTickScheduler()
{
	clear();
}

bool PcLegacyTickScheduler::schedule(int_t x, int_t y, int_t z, int_t blockId, long_t scheduledTime, int_t priority)
{
	NextTickListEntry *entry = new NextTickListEntry(x, y, z, blockId);
	entry->setScheduledTime(scheduledTime);
	(void)priority;   // 1.2.5 ticks have no priority (order stays time, then id)

	if (!uniqueEntries.add(entry))
	{
		delete entry;
		return false;
	}

	orderedEntries.insert(entry);
	attachToChunk(entry);
	return true;
}

bool PcLegacyTickScheduler::contains(int_t x, int_t y, int_t z, int_t blockId) const
{
	NextTickListEntry probe(x, y, z, blockId);
	return uniqueEntries.contains(&probe);
}

NextTickListEntry *PcLegacyTickScheduler::popNext(long_t currentTime, bool force)
{
	if (orderedEntries.empty())
		return nullptr;

	auto iterator = orderedEntries.begin();
	NextTickListEntry *entry = *iterator;
	if (!force && entry->scheduledTime > currentTime)
		return nullptr;

	orderedEntries.erase(iterator);
	uniqueEntries.remove(entry);
	detachFromChunk(entry);
	return entry;
}

std::vector<NextTickListEntry *> PcLegacyTickScheduler::getPendingForChunk(int_t chunkX, int_t chunkZ, bool remove)
{
	std::vector<NextTickListEntry *> result;
	const ChunkKey key = makeChunkKey(chunkX, chunkZ);
	auto bucketIterator = entriesByChunk.find(key);
	if (bucketIterator == entriesByChunk.end())
		return result;

	ChunkEntries &entries = bucketIterator->second;
	result.assign(entries.begin(), entries.end());
	std::sort(result.begin(), result.end(), NextTickListEntryComparator{});
	if (!remove)
		return result;

	for (NextTickListEntry *entry : result)
		detachFromCoreIndexes(entry);
	entriesByChunk.erase(bucketIterator);
	return result;
}

void PcLegacyTickScheduler::clear()
{
	for (NextTickListEntry *entry : orderedEntries)
		delete entry;

	orderedEntries.clear();
	uniqueEntries.clear();
	entriesByChunk.clear();
}

std::size_t PcLegacyTickScheduler::size() const
{
	return orderedEntries.size();
}

bool PcLegacyTickScheduler::empty() const
{
	return orderedEntries.empty();
}

PcLegacyTickScheduler::ChunkKey PcLegacyTickScheduler::makeChunkKey(int_t chunkX, int_t chunkZ)
{
	const std::uint64_t x = static_cast<std::uint32_t>(chunkX);
	const std::uint64_t z = static_cast<std::uint32_t>(chunkZ);
	return (x << 32) | z;
}

PcLegacyTickScheduler::ChunkKey PcLegacyTickScheduler::makeChunkKeyForEntry(const NextTickListEntry *entry)
{
	return makeChunkKey(JavaArithmetic::intShr(entry->xCoord, 4), JavaArithmetic::intShr(entry->zCoord, 4));
}

void PcLegacyTickScheduler::attachToChunk(NextTickListEntry *entry)
{
	entriesByChunk[makeChunkKeyForEntry(entry)].push_back(entry);
}

void PcLegacyTickScheduler::detachFromChunk(NextTickListEntry *entry)
{
	const ChunkKey key = makeChunkKeyForEntry(entry);
	auto bucketIterator = entriesByChunk.find(key);
	if (bucketIterator == entriesByChunk.end())
		return;

	ChunkEntries &entries = bucketIterator->second;
	auto entryIterator = std::find(entries.begin(), entries.end(), entry);
	if (entryIterator != entries.end())
	{
		*entryIterator = entries.back();
		entries.pop_back();
	}

	if (entries.empty())
		entriesByChunk.erase(bucketIterator);
}

void PcLegacyTickScheduler::detachFromCoreIndexes(NextTickListEntry *entry)
{
	orderedEntries.erase(entry);
	uniqueEntries.remove(entry);
}
