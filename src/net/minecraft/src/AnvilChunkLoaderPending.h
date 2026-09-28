#pragma once

#include <utility>
#include <vector>
#include <memory>

#include "ChunkCoordIntPair.h"
#include "NBTTagCompound.h"
#include "java/Type.h"

// net.minecraft.src.AnvilChunkLoaderPending
class AnvilChunkLoaderPending
{
public:
    AnvilChunkLoaderPending(const ChunkCoordIntPair &position, std::vector<byte_t> data)
        : chunkPosition(position), serializedData(std::move(data))
    {
    }

    AnvilChunkLoaderPending(const ChunkCoordIntPair &position, std::unique_ptr<NBTTagCompound> nbt)
        : chunkPosition(position), nbtRoot(std::move(nbt))
    {
    }

    ~AnvilChunkLoaderPending();

    AnvilChunkLoaderPending(const AnvilChunkLoaderPending &) = delete;
    AnvilChunkLoaderPending &operator=(const AnvilChunkLoaderPending &) = delete;

    ChunkCoordIntPair chunkPosition;
    std::unique_ptr<NBTTagCompound> nbtRoot;
    std::vector<byte_t> serializedData;
};
