#include "WorldGenTallGrass.h"

#include "Block.h"
#include "BlockFlower.h"
#include "BlockLeaves.h"
#include "World.h"
#include "platform/PlatformTuning.h"

WorldGenTallGrass::WorldGenTallGrass(int_t i, int_t j)
{
	field_28060_a = i;
	field_28059_b = j;
}

bool WorldGenTallGrass::generate(World *world, Random &random, int_t i, int_t j, int_t k)
{
	BlockFlower *flower = dynamic_cast<BlockFlower *>(Block::blocksList[field_28060_a]);
	for (int_t l = 0; ((l = world->getBlockId(i, j, k)) == 0 || l == Block::leaves->blockID) && j > 0; j--)
	{
	}
#if PLATFORM_CONSOLE_LOW || PLATFORM_WII
	const int_t maxAttempts = PLATFORM_TALL_GRASS_PLACEMENT_ATTEMPTS;
#else
	const int_t maxAttempts = (world != nullptr && world->isLimitedWorld()) ? 64 : PLATFORM_TALL_GRASS_PLACEMENT_ATTEMPTS;
#endif
	for (int_t i1 = 0; i1 < maxAttempts; i1++)
	{
		int_t j1 = random.nextIntOffset(i, 8);
		int_t k1 = random.nextIntOffset(j, 4);
		int_t l1 = random.nextIntOffset(k, 8);
		if (world->isAirBlock(j1, k1, l1) && flower && flower->canBlockStay(world, j1, k1, l1))
			setBlockAndMetadata(world, j1, k1, l1, field_28060_a, field_28059_b);
	}
	return true;
}
