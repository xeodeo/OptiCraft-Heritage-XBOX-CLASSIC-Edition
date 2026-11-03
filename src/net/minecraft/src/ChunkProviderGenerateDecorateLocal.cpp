// Chunk-local decoration (PLATFORM_CHUNK_LOCAL_DECORATION).
//
// Vanilla decorates a chunk in populate(), once its three +x/+z neighbours
// exist, because decoration is centred on the 2x2 seam (every position is
// chunk origin + 8 + rand(16)) and may spill into any of the four. On console
// that means a chunk is published and meshed bare, and its trees, ores and
// plants arrive later through setBlock on a live chunk: a lighting job and a
// remesh per block, spread over ticks, visible as trees popping in.
//
// Here the same BiomeDecorator stages that only write blocks -- ores through
// cacti, plus the biome extras -- run inside generation, against the flat
// block buffer, before the Chunk exists. Passing (blockX - 8, blockZ - 8) as
// the decoration origin lands the decorator's own +8 exactly on this chunk, so
// no generator changes; World::chunkLocalDecoration answers its reads and
// writes from the buffer and clips whatever crosses the border. Structure
// placement stays in deferred populate; cached component bounds only reserve
// tree space here. Lakes, dungeons, springs and animal groups stay deferred.
//
// Trade-off: worldgen changes. Trees on a chunk border are cut at it (the
// early Pocket Edition look), veins are clipped, and the decoration RNG no
// longer shares its stream with structures and lakes. Saved chunks are
// unaffected; chunks generated from now on decorate this way.

#include "ChunkProviderGenerate.h"

#if PLATFORM_CHUNK_LOCAL_DECORATION

#include <cstring>

#include "BiomeDecorator.h"
#include "BiomeGenBase.h"
#include "MapGenStructure.h"
#include "StructureBoundingBox.h"
#include "World.h"
#include "platform/world/ChunkLocalDecorationTarget.h"
#include "java/Arithmetic.h"
#include "platform/WorldLoadTrace.h"

namespace
{
	// World-bound scope so an exception inside a generator cannot leave the
	// world routing block access to a buffer that is about to go away.
	struct ChunkLocalDecorationScope
	{
		World *world;
		ChunkLocalDecorationScope(World *w, int_t chunkX, int_t chunkZ, byte_t *blocks, byte_t *metadata,
		                          const std::vector<StructureBoundingBox> *structureBounds)
			: world(w)
		{
			world->beginChunkLocalDecoration(chunkX, chunkZ, blocks, metadata, structureBounds);
		}
		~ChunkLocalDecorationScope() { world->endChunkLocalDecoration(); }
		ChunkLocalDecorationScope(const ChunkLocalDecorationScope &) = delete;
		ChunkLocalDecorationScope &operator=(const ChunkLocalDecorationScope &) = delete;
	};
}

void ChunkProviderGenerate::seedChunkDecoration(Random &random, int_t chunkX, int_t chunkZ) const
{
	// Same derivation populate() uses for its per-chunk stream, so decoration
	// is a function of (seed, chunk) alone.
	const long_t worldSeed = worldObj->getRandomSeed();
	random.setSeed(worldSeed);
	const long_t xMultiplier = (random.nextLong() / 2LL) * 2LL + 1LL;
	const long_t zMultiplier = (random.nextLong() / 2LL) * 2LL + 1LL;
	const ulong_t seedBits =
		static_cast<ulong_t>(static_cast<long_t>(chunkX)) * static_cast<ulong_t>(xMultiplier) +
		static_cast<ulong_t>(static_cast<long_t>(chunkZ)) * static_cast<ulong_t>(zMultiplier) ^
		static_cast<ulong_t>(worldSeed);
	random.setSeed(JavaArithmetic::longFromBits(seedBits));
}

void ChunkProviderGenerate::decorateChunkLocal(int_t chunkX, int_t chunkZ, byte_t *blocks,
	byte_t *metadata, const byte_t biomeIds[256])
{
	if (worldObj == nullptr)
		return;
	WorldLoadTrace::step("decorate");

	std::memset(metadata, 0, ChunkLocalDecorationTarget::BLOCK_COUNT);

	// The chunk's own centre column, where populate() would have read the seam
	// biome at (blockX + 16, blockZ + 16).
	BiomeGenBase *biome = BiomeGenBase::biomeList[biomeIds[8 * 16 + 8] & 0xff];
	if (biome == nullptr)
		return;

	Random random(0);
	seedChunkDecoration(random, chunkX, chunkZ);

	const int_t originX = JavaArithmetic::intSub(JavaArithmetic::intMul(chunkX, 16), 8);
	const int_t originZ = JavaArithmetic::intSub(JavaArithmetic::intMul(chunkZ, 16), 8);

	std::vector<StructureBoundingBox> structureBounds;
	if (mapFeaturesEnabled)
	{
		const int_t minX = JavaArithmetic::intMul(chunkX, 16);
		const int_t minZ = JavaArithmetic::intMul(chunkZ, 16);
		const StructureBoundingBox chunkBounds(
			minX, 0, minZ, JavaArithmetic::intAdd(minX, 15),
			ChunkLocalDecorationTarget::BUFFER_HEIGHT - 1, JavaArithmetic::intAdd(minZ, 15));
		MapGenStructure *generators[4];
		const int_t count = structureGenerators(generators);
		for (int_t index = 0; index < count; ++index)
			generators[index]->appendIntersectingComponentBounds(chunkBounds, structureBounds, 1);
	}

	ChunkLocalDecorationScope scope(worldObj, chunkX, chunkZ, blocks, metadata, &structureBounds);

	if (biome->biomeDecorator != nullptr)
	{
		// Unsliced on purpose: BiomeDecorator is per-biome state, and the
		// deferred populate of another chunk may run its springs pass on the
		// same decorator between generation ticks.
		biome->biomeDecorator->beginDecoration(worldObj, random, originX, originZ,
			BiomeDecorator::DecorationPass::Vegetation);
		try
		{
			while (!biome->biomeDecorator->advanceDecoration())
			{
			}
		}
		catch (...)
		{
			biome->biomeDecorator->finishDecoration();
			throw;
		}
		biome->biomeDecorator->finishDecoration();
	}

	int_t extraIndex = 0;
	while (!biome->advanceDecorationExtra(worldObj, random, originX, originZ, extraIndex))
	{
	}
}

#endif // PLATFORM_CHUNK_LOCAL_DECORATION
