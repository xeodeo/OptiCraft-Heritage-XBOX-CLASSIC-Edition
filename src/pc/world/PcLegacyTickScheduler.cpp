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
	entry->func_82753_a(priority);

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
	const int_t minX = JavaArithmetic::intSub(JavaArithmetic::intShl(chunkX, 4), 2);
	const int_t maxX = JavaArithmetic::intAdd(minX, 16 + 2);
	const int_t minZ = JavaArithmetic::intSub(JavaArithmetic::intShl(chunkZ, 4), 2);
	const int_t maxZ = JavaArithmetic::intAdd(minZ, 16 + 2);

	for (int_t dx = -1; dx <= 1; ++dx)
	{
		for (int_t dz = -1; dz <= 1; ++dz)
		{
			const ChunkKey key = makeChunkKey(JavaArithmetic::intAdd(chunkX, dx), JavaArithmetic::intAdd(chunkZ, dz));
			auto bucketIterator = entriesByChunk.find(key);
			if (bucketIterator == entriesByChunk.end())
				continue;

			const ChunkEntries &entries = bucketIterator->second;
			for (NextTickListEntry *entry : entries)
			{
				if (entry != nullptr &&
				    entry->xCoord >= minX && entry->xCoord < maxX &&
				    entry->zCoord >= minZ && entry->zCoord < maxZ)
				{
					result.push_back(entry);
				}
			}
		}
	}

	std::sort(result.begin(), result.end(), NextTickListEntryComparator{});
	if (!remove)
		return result;

	for (NextTickListEntry *entry : result)
	{
		detachFromCoreIndexes(entry);
		detachFromChunk(entry);
	}
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
	ChunkEntries &entries = entriesByChunk[makeChunkKeyForEntry(entry)];
	entry->chunkBucketIndex = entries.size();
	entries.push_back(entry);
}

void PcLegacyTickScheduler::detachFromChunk(NextTickListEntry *entry)
{
	const ChunkKey key = makeChunkKeyForEntry(entry);
	auto bucketIterator = entriesByChunk.find(key);
	if (bucketIterator == entriesByChunk.end())
		return;

	ChunkEntries &entries = bucketIterator->second;
	const std::size_t idx = entry->chunkBucketIndex;
	if (idx < entries.size() && entries[idx] == entry)
	{
		NextTickListEntry *last = entries.back();
		entries[idx] = last;
		last->chunkBucketIndex = idx;
		entries.pop_back();
		entry->chunkBucketIndex = static_cast<std::size_t>(-1);
	}

	if (entries.empty())
		entriesByChunk.erase(bucketIterator);
}

void PcLegacyTickScheduler::detachFromCoreIndexes(NextTickListEntry *entry)
{
	orderedEntries.erase(entry);
	uniqueEntries.remove(entry);
}
