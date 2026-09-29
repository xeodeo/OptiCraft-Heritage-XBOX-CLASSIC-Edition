#include "platform/world/PlatformBlockCollisionSweeper.h"

#if PLATFORM_FAST_BLOCK_COLLISIONS || PLATFORM_EARLY_COLLISION_EXIT || PLATFORM_FLOAT_COLLISION_SWEEP

#include <algorithm>

#include "java/Arithmetic.h"
#include "net/minecraft/src/AxisAlignedBB.h"
#include "net/minecraft/src/Block.h"
#include "net/minecraft/src/Chunk.h"
#include "net/minecraft/src/ExtendedBlockStorage.h"
#include "net/minecraft/src/MathHelper.h"
#include "net/minecraft/src/NibbleArray.h"
#include "net/minecraft/src/World.h"
#include "net/minecraft/src/WorldHeight.h"
#include "platform/world/PlatformBlockCollisionInfo.h"
#include "platform/world/PlatformBlockCollisionMath.h"

namespace
{
    bool isFastUnitCollisionBlock(int_t blockId)
    {
#if PLATFORM_EARLY_UNIT_CUBE_COLLISION_TEST
        return platformGetBlockCollisionInfo(blockId).unitCube;
#else
        (void)blockId;
        return false;
#endif
    }

    template <typename Visitor>
    bool scanCollisionBlocks(World *world, AxisAlignedBB *mask, Visitor &&visitor)
    {
        if (world == nullptr || mask == nullptr)
            return false;

        const int_t minX = MathHelper::floor_double(mask->minX);
        const int_t maxX = MathHelper::floor_double(mask->maxX + 1.0);
        const int_t minY = std::max(0, MathHelper::floor_double(mask->minY) - 1);
        const int_t maxY = std::min(WorldHeight::HEIGHT, MathHelper::floor_double(mask->maxY + 1.0));
        const int_t minZ = MathHelper::floor_double(mask->minZ);
        const int_t maxZ = MathHelper::floor_double(mask->maxZ + 1.0);

        if (minY >= maxY)
            return false;

        const PlatformCollisionBounds maskBounds{
            mask->minX, mask->minY, mask->minZ,
            mask->maxX, mask->maxY, mask->maxZ
        };

        int_t cachedChunkX = 0;
        int_t cachedChunkZ = 0;
        Chunk *cachedChunk = nullptr;
        bool cacheValid = false;

        for (int_t x = minX; x < maxX; ++x)
        {
            if (x < -30000000 || x >= 30000000)
                continue;

            const int_t chunkX = JavaArithmetic::intShr(x, 4);
            const int_t localX = x & 15;

            for (int_t z = minZ; z < maxZ; ++z)
            {
                if (z < -30000000 || z >= 30000000)
                    continue;

                const int_t chunkZ = JavaArithmetic::intShr(z, 4);
                if (!cacheValid || chunkX != cachedChunkX || chunkZ != cachedChunkZ)
                {
                    cachedChunkX = chunkX;
                    cachedChunkZ = chunkZ;
                    cacheValid = true;
                    cachedChunk = world->getChunkIfExists(chunkX, chunkZ);
#if PLATFORM_ASYNC_CHUNK_GENERATION
                    if (cachedChunk == nullptr)
                    {
                        cachedChunk = world->getChunkFromChunkCoords(chunkX, chunkZ);
                        if (cachedChunk != nullptr && cachedChunk->isEmptyChunk())
                            cachedChunk = nullptr;
                    }
#endif
                }

                if (cachedChunk == nullptr)
                    continue;

                const int_t localZ = z & 15;
                int_t y = minY;
                while (y < maxY)
                {
                    const int_t sectionY = JavaArithmetic::intShr(y, 4);
                    const int_t sectionEndY = std::min(maxY, static_cast<int_t>(platformCollisionNextSectionY(y)));
                    const ExtendedBlockStorage *storage = cachedChunk->getBlockStorage(sectionY);
                    if (storage == nullptr || storage->getIsEmpty())
                    {
                        y = sectionEndY;
                        continue;
                    }

                    const std::vector<byte_t> &lsb = storage->func_48692_g();
                    const NibbleArray *msb = storage->getBlockMSBArray();
                    for (; y < sectionEndY; ++y)
                    {
                        const int_t localY = y & 15;
                        const int_t storageIndex = platformCollisionStorageIndex(localX, localY, localZ);
                        int_t blockId = lsb[static_cast<std::size_t>(storageIndex)] & 0xff;
                        if (msb != nullptr)
                            blockId |= msb->get(localX, localY, localZ) << 8;

                        if (blockId <= 0 || blockId >= Block::BLOCK_REGISTRY_SIZE)
                            continue;

                        Block *block = Block::blocksList[blockId];
                        if (block != nullptr && visitor(block, blockId, x, y, z, maskBounds))
                            return true;
                    }
                }
            }
        }

        return false;
    }
}

void platformCollectBlockCollisions(World *world, AxisAlignedBB *mask, std::vector<AxisAlignedBB *> &collisions)
{
    scanCollisionBlocks(world, mask,
        [&](Block *block, int_t blockId, int_t x, int_t y, int_t z, const PlatformCollisionBounds &maskBounds)
        {
            if (isFastUnitCollisionBlock(blockId))
            {
                if (platformUnitCubeIntersects(maskBounds, x, y, z))
                {
                    collisions.push_back(AxisAlignedBB::getBoundingBoxFromPool(
                        static_cast<double>(x), static_cast<double>(y), static_cast<double>(z),
                        static_cast<double>(x) + 1.0, static_cast<double>(y) + 1.0, static_cast<double>(z) + 1.0));
                }
            }
            else
            {
                block->getCollidingBoundingBoxes(world, x, y, z, mask, collisions);
            }

            return false;
        });
}

bool platformHasBlockCollision(World *world, AxisAlignedBB *mask, std::vector<AxisAlignedBB *> &scratch)
{
    scratch.clear();
    return scanCollisionBlocks(world, mask,
        [&](Block *block, int_t blockId, int_t x, int_t y, int_t z, const PlatformCollisionBounds &maskBounds)
        {
            if (isFastUnitCollisionBlock(blockId))
                return platformUnitCubeIntersects(maskBounds, x, y, z);

            scratch.clear();
            block->getCollidingBoundingBoxes(world, x, y, z, mask, scratch);
            return !scratch.empty();
        });
}

#if PLATFORM_FLOAT_COLLISION_SWEEP
void platformAppendCollisionSweepBox(PlatformCollisionSweep &sweep, const AxisAlignedBB *box)
{
    // The six double subtractions here are the only libgcc work a non-unit
    // box costs; unit cubes never reach this path.
    PlatformCollisionSweepBox local;
    local.minX = static_cast<float>(box->minX - static_cast<double>(sweep.originX));
    local.minY = static_cast<float>(box->minY - static_cast<double>(sweep.originY));
    local.minZ = static_cast<float>(box->minZ - static_cast<double>(sweep.originZ));
    local.maxX = static_cast<float>(box->maxX - static_cast<double>(sweep.originX));
    local.maxY = static_cast<float>(box->maxY - static_cast<double>(sweep.originY));
    local.maxZ = static_cast<float>(box->maxZ - static_cast<double>(sweep.originZ));
    sweep.boxes.push_back(local);
}

void platformCollectBlockCollisionSweep(World *world, AxisAlignedBB *mask, PlatformCollisionSweep &sweep)
{
    sweep.boxes.clear();
    if (mask == nullptr)
        return;

    sweep.originX = MathHelper::floor_double(mask->minX);
    sweep.originY = MathHelper::floor_double(mask->minY);
    sweep.originZ = MathHelper::floor_double(mask->minZ);

    // Rebased once per query, then every unit cube is tested in float.
    PlatformCollisionSweepBox localMask;
    localMask.minX = static_cast<float>(mask->minX - static_cast<double>(sweep.originX));
    localMask.minY = static_cast<float>(mask->minY - static_cast<double>(sweep.originY));
    localMask.minZ = static_cast<float>(mask->minZ - static_cast<double>(sweep.originZ));
    localMask.maxX = static_cast<float>(mask->maxX - static_cast<double>(sweep.originX));
    localMask.maxY = static_cast<float>(mask->maxY - static_cast<double>(sweep.originY));
    localMask.maxZ = static_cast<float>(mask->maxZ - static_cast<double>(sweep.originZ));

    // Non-unit blocks still answer through their virtual getCollidingBoundingBoxes
    // with pooled double boxes; those are rebased on the way in.
    static std::vector<AxisAlignedBB *> s_shapedScratch;

    scanCollisionBlocks(world, mask,
        [&](Block *block, int_t blockId, int_t x, int_t y, int_t z, const PlatformCollisionBounds &)
        {
            if (isFastUnitCollisionBlock(blockId))
            {
                const float minX = static_cast<float>(x - sweep.originX);
                const float minY = static_cast<float>(y - sweep.originY);
                const float minZ = static_cast<float>(z - sweep.originZ);
                const float maxX = minX + 1.0f;
                const float maxY = minY + 1.0f;
                const float maxZ = minZ + 1.0f;
                if (localMask.maxX > minX && localMask.minX < maxX &&
                    localMask.maxY > minY && localMask.minY < maxY &&
                    localMask.maxZ > minZ && localMask.minZ < maxZ)
                {
                    sweep.boxes.push_back(PlatformCollisionSweepBox{minX, minY, minZ, maxX, maxY, maxZ});
                }
            }
            else
            {
                s_shapedScratch.clear();
                block->getCollidingBoundingBoxes(world, x, y, z, mask, s_shapedScratch);
                for (const AxisAlignedBB *box : s_shapedScratch)
                    platformAppendCollisionSweepBox(sweep, box);
            }

            return false;
        });
}
#endif

#endif
