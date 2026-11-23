#pragma once

#include <vector>

#include "FlatGeneratorInfo.h"
#include "IChunkProvider.h"
#include "java/Random.h"
#include "java/String.h"

class MapGenStructure;
class World;
class WorldGenLakes;

// net.minecraft.src.ChunkProviderFlat (Java 1.5.2)
class ChunkProviderFlat : public IChunkProvider
{
public:
	ChunkProviderFlat(World *world, long_t seed, bool useStructures, const jstring &generatorOptions = "");
	~ChunkProviderFlat() override;

	Chunk *prepareChunk(int_t x, int_t z) override;
	Chunk *loadChunk(int_t x, int_t z) override;
	Chunk *provideChunk(int_t x, int_t z) override;
	bool chunkExists(int_t x, int_t z) override;
	void populate(IChunkProvider *provider, int_t x, int_t z) override;
	bool saveChunks(bool saveAll, IProgressUpdate *progress) override;
	bool unload100OldestChunks() override;
	bool canSave() override;
	jstring makeString() override;
	std::vector<SpawnListEntry> *getPossibleCreatures(const EnumCreatureType &type, int_t x, int_t y, int_t z) override;
	ChunkPosition *findClosestStructure(World *world, const jstring &name, int_t x, int_t y, int_t z) override;

private:
	World *worldObj;
	Random random;
	byte_t layerBlocks[256];
	byte_t layerMeta[256];
	FlatGeneratorInfo flatGeneratorInfo;
	std::vector<MapGenStructure *> structureGenerators;
	bool hasDecoration;
	bool hasDungeons;
	WorldGenLakes *waterLakeGenerator;
	WorldGenLakes *lavaLakeGenerator;
};
