#include "BiomeGenHills.h"

#include "Block.h"
#include "World.h"
#include "WorldGenMinable.h"
#include "java/Random.h"

BiomeGenHills::BiomeGenHills()
    : BiomeGenBase(), theWorldGenerator(nullptr)
{
}

BiomeGenHills::~BiomeGenHills()
{
    delete theWorldGenerator;
    theWorldGenerator = nullptr;
}

void BiomeGenHills::decorate(World *world, Random &random, int_t chunkX, int_t chunkZ)
{
    BiomeGenBase::decorate(world, random, chunkX, chunkZ);

    if (world == nullptr)
        return;

    const int_t stoneId = Block::stone != nullptr ? Block::stone->blockID : 1;
    const int_t emeraldId = Block::oreEmerald != nullptr ? Block::oreEmerald->blockID : 129;
    const int_t silverfishId = Block::silverfish != nullptr ? Block::silverfish->blockID : 97;

    const int_t emeraldVeins = 3 + random.nextInt(6);
    for (int_t i = 0; i < emeraldVeins; ++i)
    {
        const int_t x = chunkX + random.nextInt(16);
        const int_t y = random.nextInt(28) + 4;
        const int_t z = chunkZ + random.nextInt(16);
        world->replaceBlockForPopulation(x, y, z, stoneId, emeraldId);
    }

    if (theWorldGenerator == nullptr)
        theWorldGenerator = new WorldGenMinable(silverfishId, 8);

    for (int_t i = 0; i < 7; ++i)
    {
        const int_t x = chunkX + random.nextInt(16);
        const int_t y = random.nextInt(64);
        const int_t z = chunkZ + random.nextInt(16);
        theWorldGenerator->generate(world, random, x, y, z);
    }
}
