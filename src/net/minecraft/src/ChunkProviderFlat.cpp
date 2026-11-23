#include "ChunkProviderFlat.h"

#include <cstring>

#include "BiomeGenBase.h"
#include "Block.h"
#include "Chunk.h"
#include "ExtendedBlockStorage.h"
#include "MapGenMineshaft.h"
#include "MapGenScatteredFeature.h"
#include "MapGenStronghold.h"
#include "MapGenStructure.h"
#include "MapGenVillage.h"
#include "World.h"
#include "WorldChunkManager.h"
#include "WorldGenDungeons.h"
#include "WorldGenLakes.h"
#include "WorldHeight.h"
#include "java/Arithmetic.h"

ChunkProviderFlat::ChunkProviderFlat(World *world, long_t seed, bool structures, const jstring &generatorOptions)
	: worldObj(world)
	, random(seed)
	, flatGeneratorInfo(FlatGeneratorInfo::createFlatGeneratorFromString(generatorOptions))
	, hasDecoration(false)
	, hasDungeons(false)
	, waterLakeGenerator(nullptr)
	, lavaLakeGenerator(nullptr)
{
	std::memset(layerBlocks, 0, sizeof(layerBlocks));
	std::memset(layerMeta, 0, sizeof(layerMeta));

	auto &features = flatGeneratorInfo.getWorldFeatures();
	if (structures)
	{
		if (features.find("village") != features.end())
		{
			auto villageParams = features["village"];
			if (villageParams.find("size") == villageParams.end())
				villageParams["size"] = "1";
			structureGenerators.push_back(new MapGenVillage(villageParams));
		}

		if (features.find("biome_1") != features.end())
			structureGenerators.push_back(new MapGenScatteredFeature(features["biome_1"]));

		if (features.find("mineshaft") != features.end())
			structureGenerators.push_back(new MapGenMineshaft(features["mineshaft"]));

		if (features.find("stronghold") != features.end())
			structureGenerators.push_back(new MapGenStronghold(features["stronghold"]));
	}

	hasDecoration = (features.find("decoration") != features.end());
	if (features.find("lake") != features.end() && Block::waterStill != nullptr)
		waterLakeGenerator = new WorldGenLakes(Block::waterStill->blockID);
	if (features.find("lava_lake") != features.end() && Block::lavaStill != nullptr)
		lavaLakeGenerator = new WorldGenLakes(Block::lavaStill->blockID);
	hasDungeons = (features.find("dungeon") != features.end());

	for (const FlatLayerInfo &layer : flatGeneratorInfo.getFlatLayers())
	{
		const int_t startY = layer.getMinY();
		const int_t endY = startY + layer.getLayerCount();
		for (int_t y = startY; y < endY; ++y)
		{
			if (y >= 0 && y < WorldHeight::HEIGHT)
			{
				layerBlocks[y] = static_cast<byte_t>(layer.getFillBlock() & 0xFF);
				layerMeta[y] = static_cast<byte_t>(layer.getFillBlockMeta() & 0x0F);
			}
		}
	}
}

ChunkProviderFlat::~ChunkProviderFlat()
{
	for (MapGenStructure *gen : structureGenerators)
		delete gen;
	structureGenerators.clear();
	delete waterLakeGenerator;
	delete lavaLakeGenerator;
}

Chunk *ChunkProviderFlat::prepareChunk(int_t x, int_t z)
{
	return provideChunk(x, z);
}

Chunk *ChunkProviderFlat::loadChunk(int_t x, int_t z)
{
	return provideChunk(x, z);
}

Chunk *ChunkProviderFlat::provideChunk(int_t x, int_t z)
{
	Chunk *chunk = new Chunk(worldObj, x, z);
	ExtendedBlockStorage **storageArr = chunk->getBlockStorageArray();

	for (int_t y = 0; y < WorldHeight::HEIGHT; ++y)
	{
		const int_t blockId = static_cast<int_t>(static_cast<unsigned char>(layerBlocks[y]));
		if (blockId == 0)
			continue;

		const int_t sectionY = y >> 4;
		ExtendedBlockStorage *storage = storageArr[sectionY];
		if (storage == nullptr)
		{
			storage = new ExtendedBlockStorage(sectionY << 4);
			storageArr[sectionY] = storage;
		}

		const int_t localY = y & 15;
		const int_t meta = static_cast<int_t>(static_cast<unsigned char>(layerMeta[y])) & 0x0F;
		for (int_t lx = 0; lx < 16; ++lx)
		{
			for (int_t lz = 0; lz < 16; ++lz)
			{
				storage->setExtBlockID(lx, localY, lz, blockId);
				if (meta != 0)
					storage->setExtBlockMetadata(lx, localY, lz, meta);
			}
		}
	}

	chunk->generateSkylightMap();

	std::vector<BiomeGenBase *> biomes;
	worldObj->getWorldChunkManager()->loadBlockGeneratorData(
		biomes, JavaArithmetic::intMul(x, 16), JavaArithmetic::intMul(z, 16), 16, 16);
	std::vector<byte_t> &biomeArray = chunk->getBiomeArray();
	for (std::size_t i = 0; i < biomeArray.size() && i < biomes.size(); ++i)
		biomeArray[i] = biomes[i] == nullptr ? 0 : static_cast<byte_t>(biomes[i]->biomeID);

	for (MapGenStructure *gen : structureGenerators)
		gen->generate(this, worldObj, x, z, nullptr);

	chunk->generateSkylightMap();
	return chunk;
}

bool ChunkProviderFlat::chunkExists(int_t, int_t)
{
	return true;
}

void ChunkProviderFlat::populate(IChunkProvider *, int_t x, int_t z)
{
	const int_t blockX = JavaArithmetic::intMul(x, 16);
	const int_t blockZ = JavaArithmetic::intMul(z, 16);
	BiomeGenBase *biome = worldObj->getBiomeGenForCoords(blockX + 16, blockZ + 16);
	bool hasVillage = false;

	random.setSeed(worldObj->getRandomSeed());
	const long_t oddX = JavaArithmetic::longAdd(JavaArithmetic::longMul(random.nextLong() / 2LL, 2LL), 1LL);
	const long_t oddZ = JavaArithmetic::longAdd(JavaArithmetic::longMul(random.nextLong() / 2LL, 2LL), 1LL);
	const long_t coordSeed = JavaArithmetic::longAdd(JavaArithmetic::longMul(static_cast<long_t>(x), oddX),
	                                                 JavaArithmetic::longMul(static_cast<long_t>(z), oddZ));
	random.setSeed(JavaArithmetic::longFromBits(static_cast<ulong_t>(coordSeed) ^ static_cast<ulong_t>(worldObj->getRandomSeed())));

	for (MapGenStructure *gen : structureGenerators)
	{
		const bool generated = gen->generateStructuresInChunk(worldObj, random, x, z);
		if (dynamic_cast<MapGenVillage *>(gen) != nullptr)
			hasVillage = hasVillage || generated;
	}

	if (waterLakeGenerator != nullptr && !hasVillage && random.nextInt(4) == 0)
	{
		const int_t lx = blockX + random.nextInt(16) + 8;
		const int_t ly = random.nextInt(128);
		const int_t lz = blockZ + random.nextInt(16) + 8;
		waterLakeGenerator->generate(worldObj, random, lx, ly, lz);
	}

	if (lavaLakeGenerator != nullptr && !hasVillage && random.nextInt(8) == 0)
	{
		const int_t lx = blockX + random.nextInt(16) + 8;
		const int_t ly = random.nextInt(random.nextInt(120) + 8);
		const int_t lz = blockZ + random.nextInt(16) + 8;
		if (ly < 63 || random.nextInt(10) == 0)
			lavaLakeGenerator->generate(worldObj, random, lx, ly, lz);
	}

	if (hasDungeons)
	{
		WorldGenDungeons dungeonGen;
		for (int_t i = 0; i < 8; ++i)
		{
			const int_t dx = blockX + random.nextInt(16) + 8;
			const int_t dy = random.nextInt(128);
			const int_t dz = blockZ + random.nextInt(16) + 8;
			dungeonGen.generate(worldObj, random, dx, dy, dz);
		}
	}

	if (hasDecoration && biome != nullptr)
	{
		biome->decorate(worldObj, random, blockX, blockZ);
	}
	if (modPopulateHook != nullptr)
		modPopulateHook(worldObj, x, z);
}

bool ChunkProviderFlat::saveChunks(bool, IProgressUpdate *)
{
	return true;
}

bool ChunkProviderFlat::unload100OldestChunks()
{
	return false;
}

bool ChunkProviderFlat::canSave()
{
	return true;
}

jstring ChunkProviderFlat::makeString()
{
	return "FlatLevelSource";
}

std::vector<SpawnListEntry> *ChunkProviderFlat::getPossibleCreatures(const EnumCreatureType &type, int_t x, int_t, int_t z)
{
	BiomeGenBase *biome = worldObj->getBiomeGenForCoords(x, z);
	return biome == nullptr ? nullptr : biome->getSpawnableList(type);
}

ChunkPosition *ChunkProviderFlat::findClosestStructure(World *world, const jstring &name, int_t x, int_t y, int_t z)
{
	if (name == "Stronghold")
	{
		for (MapGenStructure *gen : structureGenerators)
		{
			if (dynamic_cast<MapGenStronghold *>(gen) != nullptr)
				return gen->getNearestInstance(world, x, y, z);
		}
	}
	return nullptr;
}
