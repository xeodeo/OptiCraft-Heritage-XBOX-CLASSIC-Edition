#include "WorldGenFlowers.h"

#include "Block.h"
#include "BlockFlower.h"
#include "World.h"
#include "platform/PlatformTuning.h"

WorldGenFlowers::WorldGenFlowers(int_t i)
{
	plantBlockId = i;
}

bool WorldGenFlowers::generate(World *world, Random &random, int_t i, int_t j, int_t k)
{
	// plantBlockId is fixed for the generator's lifetime; the cast was inside
	BlockFlower *flower = dynamic_cast<BlockFlower *>(Block::blocksList[plantBlockId]);
#if PLATFORM_CONSOLE_LOW || PLATFORM_WII
	const int_t maxAttempts = PLATFORM_FLOWER_PLACEMENT_ATTEMPTS;
#else
	const int_t maxAttempts = (world != nullptr && world->isLimitedWorld()) ? 48 : PLATFORM_FLOWER_PLACEMENT_ATTEMPTS;
#endif
	for (int_t l = 0; l < maxAttempts; l++)
	{
		int_t i1 = random.nextIntOffset(i, 8);
		int_t j1 = random.nextIntOffset(j, 4);
		int_t k1 = random.nextIntOffset(k, 8);
		if (world->isAirBlock(i1, j1, k1) && flower && flower->canBlockStay(world, i1, j1, k1))
			world->setBlock(i1, j1, k1, plantBlockId);
	}
	return true;
}
