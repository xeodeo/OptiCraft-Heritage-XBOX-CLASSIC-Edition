#pragma once
#include "NoiseBuffer.h"

#include <cstddef>
#include <memory>
#include <vector>

#include "IChunkProvider.h"
#include "java/Type.h"
#include "java/Random.h"
#include "java/String.h"
#include "NoiseGeneratorOctaves.h"
#include "platform/PlatformTuning.h"

#if PLATFORM_USE_HEIGHTMAP_TERRAIN
#include "LiteTerrainShape.h"
#endif

class World;
class Chunk;
class BiomeGenBase;
class MapGenBase;
class MapGenMineshaft;
class MapGenStructure;
class MapGenVillage;
class MapGenStronghold;
class MapGenScatteredFeature;
class IProgressUpdate;
class WorldChunkManager;

// net.minecraft.src.ChunkProviderGenerate
class ChunkProviderGenerate : public IChunkProvider
{
public:
	ChunkProviderGenerate(World *world, long_t l, bool mapFeaturesEnabled = true,
	                      bool isolatedBiomeSource = false);
	~ChunkProviderGenerate() override;

#if !PLATFORM_USE_HEIGHTMAP_TERRAIN
	void generateTerrain(int_t i, int_t j, byte_t *abyte0, BiomeGenBase **abiomegenbase, const biome_noise_real_t *ad);
#else
	// Cheap all-float 2D heightmap terrain path for weak consoles.  Drop-in
	// replacement for generateTerrain(); see ChunkProviderGenerateLite.cpp.
	void generateTerrainHeightmap(int_t i, int_t j, byte_t *abyte0);
	void replaceBlocksForBiomeHeightmap(byte_t *blocks, BiomeGenBase **biomes);
#endif
	void replaceBlocksForBiome(int_t i, int_t j, byte_t *abyte0, BiomeGenBase **abiomegenbase);

	Chunk *prepareChunk(int_t i, int_t j) override;
	bool generateAsyncChunkData(int_t chunkX, int_t chunkZ, std::vector<byte_t> &data);
	Chunk *finishAsyncChunkData(int_t chunkX, int_t chunkZ, std::vector<byte_t> &data);
	Chunk *provideChunk(int_t i, int_t j) override;
	Chunk *loadChunk(int_t i, int_t j) override;

	bool    chunkExists(int_t i, int_t j) override;
	void    populate(IChunkProvider *ichunkprovider, int_t i, int_t j) override;
#if PLATFORM_INCREMENTAL_POPULATE
	bool    populateStep(IChunkProvider *ichunkprovider, int_t i, int_t j) override;
#endif
	bool    saveChunks(bool flag, IProgressUpdate *iprogressupdate) override;
	bool    unload100OldestChunks() override;
	bool    canSave() override;
	jstring makeString() override;
	std::vector<SpawnListEntry> *getPossibleCreatures(const EnumCreatureType &type, int_t x, int_t y, int_t z) override;

	ChunkPosition *findClosestStructure(World *world, const jstring &name, int_t x, int_t y, int_t z) override;
	void recreateStructures(int_t x, int_t z) override;

#if PLATFORM_INCREMENTAL_CHUNK_GENERATION
	bool beginGenerationTask(int_t chunkX, int_t chunkZ);
	bool advanceGenerationTask();
	void cancelGenerationTask();
	bool hasGenerationTask() const;
	int_t generationTaskX() const;
	int_t generationTaskZ() const;
	Chunk *takeGeneratedChunk();
#endif

private:
#if !PLATFORM_USE_HEIGHTMAP_TERRAIN
	TerrainNoiseBuffer &initializeNoiseField(TerrainNoiseBuffer &ad, int_t i, int_t j, int_t k,
	                                            int_t l, int_t i1, int_t j1);
	TerrainNoiseBuffer &generateNoiseField(TerrainNoiseBuffer &ad, int_t i, int_t j, int_t k,
	                                        int_t l, int_t i1, int_t j1); // compatibility alias
#endif

	void seedChunkGeneration(int_t chunkX, int_t chunkZ);
	void generateBaseTerrain(int_t chunkX, int_t chunkZ, byte_t *blocks, byte_t biomeIds[256]);
	void generateCaves(int_t chunkX, int_t chunkZ, byte_t *blocks);
	void generateRavines(int_t chunkX, int_t chunkZ, byte_t *blocks);
#if PLATFORM_INCREMENTAL_CHUNK_GENERATION
	// Sliced forms of the two sweeps above, for advanceGenerationTask(). Each
	// advances cursor by at most PLATFORM_GENERATION_SOURCE_COLUMNS_PER_STEP
	// source columns and returns true once its sweep is complete.
	bool generateCavesStep(int_t chunkX, int_t chunkZ, byte_t *blocks, int_t &cursor);
	bool generateRavinesStep(int_t chunkX, int_t chunkZ, byte_t *blocks, int_t &cursor);
	bool generateStructuresStep(int_t chunkX, int_t chunkZ, byte_t *blocks, int_t &cursor);
#endif
	// Mineshafts, villages and strongholds in one source sweep
	// (MapGenBase::generateRangeShared); the three used to walk the same 17x17
	// columns and derive the same per-column seed independently.
	void generateStructures(int_t chunkX, int_t chunkZ, byte_t *blocks);
	int_t structureGenerators(MapGenStructure *out[4]) const;
	// Bounds the three structure caches around the chunk just generated; see
	// PLATFORM_STRUCTURE_START_RETENTION_BLOCKS.
	void trimStructureStarts(int_t chunkX, int_t chunkZ);
#if PLATFORM_CHUNK_LOCAL_DECORATION
	// Vegetation/ore decoration against the generation buffer, before the
	// Chunk exists; see ChunkProviderGenerateDecorateLocal.cpp. metadata is a
	// byte per block in the blocks layout and is cleared here first.
	void decorateChunkLocal(int_t chunkX, int_t chunkZ, byte_t *blocks, byte_t *metadata,
	                        const byte_t biomeIds[256]);
	void seedChunkDecoration(Random &random, int_t chunkX, int_t chunkZ) const;
#endif
	Chunk *buildGeneratedChunk(int_t chunkX, int_t chunkZ, const byte_t biomeIds[256],
	                           const byte_t *blocks, std::size_t blockCount,
	                           const byte_t *metadata = nullptr);
	void finishGeneratedChunkLighting(Chunk *chunk);
	WorldChunkManager *generationWorldChunkManager() const;

	// C++ constructs members in declaration order. Keep the Java RNG/noise order:
	// every NoiseGeneratorOctaves constructor consumes rand immediately.
	Random rand;
#if !PLATFORM_USE_HEIGHTMAP_TERRAIN
	// Only initializeNoiseField() and replaceBlocksForBiome() read these. The
	// heightmap path samples its own 2D noise and derives stoneNoise from it, so
	// on that profile the seven generators (64 permutation tables, ~17k Random
	// draws per world open) would be built and never touched.
	NoiseGeneratorOctaves field_912_k;
	NoiseGeneratorOctaves field_911_l;
	NoiseGeneratorOctaves field_910_m;
	NoiseGeneratorOctaves field_909_n;

public:
	NoiseGeneratorOctaves field_922_a;
	NoiseGeneratorOctaves field_921_b;
	NoiseGeneratorOctaves mobSpawnerNoise;

private:
#endif
	World *worldObj;
	std::unique_ptr<WorldChunkManager> isolatedWorldChunkManager;
	bool mapFeaturesEnabled;
	std::vector<byte_t> blocksScratch;
#if PLATFORM_CHUNK_LOCAL_DECORATION
	std::vector<byte_t> metadataScratch;
#endif

	TerrainNoiseBuffer field_4180_q;
	TerrainNoiseBuffer stoneNoise;

	MapGenBase *caveGenerator;
	MapGenBase *ravineGenerator;
	MapGenMineshaft *mineshaftGenerator;
	MapGenVillage *villageGenerator;
	MapGenStronghold *strongholdGenerator;
	MapGenScatteredFeature *scatteredFeatureGenerator;

	std::vector<BiomeGenBase *> biomesForGeneration;
#if PLATFORM_USE_HEIGHTMAP_TERRAIN
	std::vector<BiomeGenBase *> liteBiomeHalo;
	std::vector<LiteTerrain::BiomeBlendSample> liteBiomeSamples;
	byte_t liteColumnTop[256] = {};
	byte_t liteTerrainHeight[256] = {};
#endif

	TerrainNoiseBuffer field_4185_d;
	TerrainNoiseBuffer field_4184_e;
	TerrainNoiseBuffer field_4183_f;
	TerrainNoiseBuffer field_4182_g;
	TerrainNoiseBuffer field_4181_h;
	std::vector<float> biomeWeights;
#if PLATFORM_FAST_BIOME_BLEND
	std::vector<float> biomeInverseHeightDenominators;
#endif
	int_t field_914_i[32][32];

	BiomeNoiseBuffer generatedTemperatures;

#if PLATFORM_INCREMENTAL_CHUNK_GENERATION
	enum class GenerationStage
	{
		BaseTerrain,
		Caves,
		Ravines,
		Structures,
#if PLATFORM_CHUNK_LOCAL_DECORATION
		Decorate,
#endif
		BuildChunk,
		Skylight,
		Done
	};

	struct GenerationTask
	{
		bool active = false;
		int_t chunkX = 0;
		int_t chunkZ = 0;
		GenerationStage stage = GenerationStage::BaseTerrain;
		// Position within the current stage's MapGenBase source sweep. Reset when
		// a sweep stage is entered; see MapGenBase::generateRange.
		int_t sourceCursor = 0;
		std::vector<byte_t> blocks;
#if PLATFORM_CHUNK_LOCAL_DECORATION
		std::vector<byte_t> metadata;
#endif
		byte_t biomeIds[256] = {};
		Chunk *chunk = nullptr;
	};

	GenerationTask generationTask;
#endif

#if PLATFORM_INCREMENTAL_POPULATE
	enum class PopulateStage
	{
		Structures,
		WaterLake,
		LavaLake,
		Dungeons,
		BiomeDecoration,
		BiomeDecorationExtras,
		Spawning,
		Snow,
		Done
	};

	struct PopulateTask
	{
		bool active = false;
		int_t chunkX = 0;
		int_t chunkZ = 0;
		int_t blockX = 0;
		int_t blockZ = 0;
		BiomeGenBase *biome = nullptr;
		bool villageGenerated = false;
		bool decorationStarted = false;
		Random random{0};
		PopulateStage stage = PopulateStage::Structures;
		int_t index = 0;
		long long totalNs = 0;
	};

	void beginPopulateTask(int_t i, int_t j);
	bool advancePopulateTask();
	void finishPopulateTask();
	PopulateTask populateTask;
#endif
};
