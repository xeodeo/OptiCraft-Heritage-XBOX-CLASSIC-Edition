#include "PortalTransferManager.h"

#include "Block.h"
#include "Chunk.h"
#include "ChunkCoordIntPair.h"
#include "CompressedStreamTools.h"
#include "Entity.h"
#include "EntityList.h"
#include "ISaveHandler.h"
#include "MathHelper.h"
#include "NBTTagCompound.h"
#include "NBTTagList.h"
#include "RegionFile.h"
#include "RegionFileCache.h"
#include "World.h"
#include "WorldProvider.h"
#include "platform/Log.h"
#include "platform/Storage.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <memory>
#include <streambuf>
#include <vector>

namespace
{
inline double clampDouble(double v, double minVal, double maxVal)
{
    return v < minVal ? minVal : (v > maxVal ? maxVal : v);
}

class VectorInputBuffer : public std::streambuf
{
public:
    VectorInputBuffer(const void *data, size_t size)
    {
        char *begin = const_cast<char *>(reinterpret_cast<const char *>(data));
        setg(begin, begin, begin + size);
    }
};

class VectorInputStream : public std::istream
{
public:
    VectorInputStream(const void *data, size_t size)
        : std::istream(nullptr), buffer(data, size)
    {
        rdbuf(&buffer);
    }
    explicit VectorInputStream(const std::vector<byte_t> &bytes)
        : VectorInputStream(bytes.data(), bytes.size()) {}
    explicit VectorInputStream(const std::vector<unsigned char> &bytes)
        : VectorInputStream(bytes.data(), bytes.size()) {}
private:
    VectorInputBuffer buffer;
};

class VectorOutputBuffer : public std::streambuf
{
public:
    explicit VectorOutputBuffer(std::vector<byte_t> &bytes) : bytes(bytes) {}

protected:
    int_type overflow(int_type ch) override
    {
        if (traits_type::eq_int_type(ch, traits_type::eof()))
            return traits_type::not_eof(ch);
        bytes.push_back((byte_t)ch);
        return ch;
    }

    std::streamsize xsputn(const char *src, std::streamsize count) override
    {
        if (count > 0)
        {
            bytes.insert(bytes.end(), reinterpret_cast<const byte_t *>(src),
                         reinterpret_cast<const byte_t *>(src) + count);
        }
        return count;
    }

private:
    std::vector<byte_t> &bytes;
};

class VectorOutputStream : public std::ostream
{
public:
    explicit VectorOutputStream(std::vector<byte_t> &bytes)
        : std::ostream(nullptr), buffer(bytes)
    {
        rdbuf(&buffer);
    }
private:
    VectorOutputBuffer buffer;
};

struct ChunkOffset
{
    int8_t dx;
    int8_t dz;
    int16_t distSq;
};

const std::vector<ChunkOffset> &getSortedChunkOffsets()
{
    static std::vector<ChunkOffset> offsets;
    if (offsets.empty())
    {
        for (int dx = -8; dx <= 8; ++dx)
        {
            for (int dz = -8; dz <= 8; ++dz)
            {
                ChunkOffset co;
                co.dx = (int8_t)dx;
                co.dz = (int8_t)dz;
                co.distSq = (int16_t)(dx * dx + dz * dz);
                offsets.push_back(co);
            }
        }
        std::sort(offsets.begin(), offsets.end(), [](const ChunkOffset &a, const ChunkOffset &b) {
            return a.distSq < b.distSq;
        });
    }
    return offsets;
}
} // namespace

std::string PortalTransferManager::getDimensionDirectory(const std::string &worldDir, int_t dimension)
{
    if (dimension == -1)
        return PlatformStorage::join(worldDir, "DIM-1");
    if (dimension == 1)
        return PlatformStorage::join(worldDir, "DIM1");
    return worldDir;
}

bool PortalTransferManager::findExistingPortalInDimension(const std::string &dimDir,
                                                         double targetX, double targetY, double targetZ,
                                                         double &outPortalX, double &outPortalY, double &outPortalZ)
{
    const int_t centerCX = MathHelper::floor_double(targetX) >> 4;
    const int_t centerCZ = MathHelper::floor_double(targetZ) >> 4;

    const auto &offsets = getSortedChunkOffsets();

    double bestDistSq = 128.0 * 128.0; // 128 block radius
    int_t bestX = 0, bestY = 0, bestZ = 0;
    bool found = false;

    for (const ChunkOffset &offset : offsets)
    {
        const int_t cx = centerCX + offset.dx;
        const int_t cz = centerCZ + offset.dz;

        const int_t minBlockDx = std::max<int_t>(0, std::abs(offset.dx) - 1) * 16;
        const int_t minBlockDz = std::max<int_t>(0, std::abs(offset.dz) - 1) * 16;
        if ((double)(minBlockDx * minBlockDx + minBlockDz * minBlockDz) >= bestDistSq)
        {
            if (found)
                break;
        }

        std::shared_ptr<RegionFile> region = RegionFileCache::acquireRegionFile(
            dimDir, cx, cz, RegionFileCache::Format::Anvil, true);
        if (region == nullptr || !region->hasChunk(cx & 31, cz & 31))
            continue;

        std::vector<byte_t> chunkData;
        RegionFile::ReadStatus status;
        if (!region->getChunkData(cx & 31, cz & 31, chunkData, &status) || chunkData.empty())
            continue;

        VectorInputStream vis(chunkData);
        std::unique_ptr<NBTTagCompound> root(CompressedStreamTools::readCompound(vis));
        if (root == nullptr)
            continue;

        NBTTagCompound *level = root->getCompoundTag("Level");
        if (level == nullptr)
            continue;

        NBTTagList *sections = level->getTagList("Sections");
        if (sections == nullptr)
            continue;

        for (int_t s = 0; s < sections->tagCount(); ++s)
        {
            NBTTagCompound *sec = dynamic_cast<NBTTagCompound *>(sections->tagAt(s));
            if (sec == nullptr)
                continue;

            const int_t secY = ((int_t)sec->getByte("Y")) & 0xff;
            if (secY < 0 || secY >= 16)
                continue;

            std::vector<byte_t> blocks = sec->getByteArray("Blocks");
            if (blocks.size() < 4096)
                continue;

            // Fast scan for portal block ID 90
            if (std::memchr(blocks.data(), 90, 4096) == nullptr)
                continue;

            for (int_t by = 0; by < 16; ++by)
            {
                for (int_t bz = 0; bz < 16; ++bz)
                {
                    for (int_t bx = 0; bx < 16; ++bx)
                    {
                        const int_t idx = (by << 8) | (bz << 4) | bx;
                        if (blocks[idx] == 90)
                        {
                            // Skip if block below in same section is also portal (find bottom)
                            if (by > 0 && blocks[((by - 1) << 8) | (bz << 4) | bx] == 90)
                                continue;

                            const int_t wx = (cx << 4) + bx;
                            const int_t wy = (secY << 4) + by;
                            const int_t wz = (cz << 4) + bz;

                            const double dX = (double)wx + 0.5 - targetX;
                            const double dY = (double)wy + 0.5 - targetY;
                            const double dZ = (double)wz + 0.5 - targetZ;
                            const double distSq = dX * dX + dY * dY + dZ * dZ;

                            if (distSq < bestDistSq)
                            {
                                bestDistSq = distSq;
                                bestX = wx;
                                bestY = wy;
                                bestZ = wz;
                                found = true;
                            }
                        }
                    }
                }
            }
        }
    }

    if (!found)
        return false;

    // Center entity within the portal frame
    double dX = (double)bestX + 0.5;
    double dY = (double)bestY;
    double dZ = (double)bestZ + 0.5;

    const int_t bestCX = bestX >> 4;
    const int_t bestCZ = bestZ >> 4;
    std::shared_ptr<RegionFile> region = RegionFileCache::acquireRegionFile(
        dimDir, bestCX, bestCZ, RegionFileCache::Format::Anvil, true);
    if (region != nullptr && region->hasChunk(bestCX & 31, bestCZ & 31))
    {
        std::vector<byte_t> chunkData;
        RegionFile::ReadStatus status;
        if (region->getChunkData(bestCX & 31, bestCZ & 31, chunkData, &status) && !chunkData.empty())
        {
            VectorInputStream vis(chunkData);
            std::unique_ptr<NBTTagCompound> root(CompressedStreamTools::readCompound(vis));
            if (root != nullptr)
            {
                NBTTagCompound *level = root->getCompoundTag("Level");
                if (level != nullptr)
                {
                    NBTTagList *sections = level->getTagList("Sections");
                    const int_t targetSecY = bestY >> 4;
                    for (int_t s = 0; sections != nullptr && s < sections->tagCount(); ++s)
                    {
                        NBTTagCompound *sec = dynamic_cast<NBTTagCompound *>(sections->tagAt(s));
                        if (sec != nullptr && (((int_t)sec->getByte("Y")) & 0xff) == targetSecY)
                        {
                            std::vector<byte_t> blocks = sec->getByteArray("Blocks");
                            if (blocks.size() >= 4096)
                            {
                                const int_t bx = bestX & 15;
                                const int_t by = bestY & 15;
                                const int_t bz = bestZ & 15;
                                if (bx > 0 && blocks[(by << 8) | (bz << 4) | (bx - 1)] == 90) dX -= 0.5;
                                if (bx < 15 && blocks[(by << 8) | (bz << 4) | (bx + 1)] == 90) dX += 0.5;
                                if (bz > 0 && blocks[(by << 8) | ((bz - 1) << 4) | bx] == 90) dZ -= 0.5;
                                if (bz < 15 && blocks[(by << 8) | ((bz + 1) << 4) | bx] == 90) dZ += 0.5;
                            }
                            break;
                        }
                    }
                }
            }
        }
    }

    outPortalX = dX;
    outPortalY = dY;
    outPortalZ = dZ;
    return true;
}

bool PortalTransferManager::writeEntityToDimensionChunk(const std::string &dimDir,
                                                       int_t chunkX, int_t chunkZ,
                                                       double destX, double destY, double destZ,
                                                       int_t targetDim, Entity *entity)
{
    std::shared_ptr<RegionFile> writeRegion = RegionFileCache::acquireRegionFile(
        dimDir, chunkX, chunkZ, RegionFileCache::Format::Anvil, false);
    if (writeRegion == nullptr || !writeRegion->hasChunk(chunkX & 31, chunkZ & 31))
        return false;

    std::vector<byte_t> rawChunk;
    RegionFile::ReadStatus status;
    if (!writeRegion->getChunkData(chunkX & 31, chunkZ & 31, rawChunk, &status) || rawChunk.empty())
        return false;

    VectorInputStream inStream(rawChunk);
    std::unique_ptr<NBTTagCompound> root(CompressedStreamTools::readCompound(inStream));
    if (root == nullptr)
        return false;

    NBTTagCompound *level = root->getCompoundTag("Level");
    if (level == nullptr)
        return false;

    NBTTagList *entitiesList = level->getTagList("Entities");
    if (entitiesList == nullptr)
    {
        entitiesList = new NBTTagList();
        level->setTag("Entities", entitiesList);
    }

    const double oldX = entity->posX;
    const double oldY = entity->posY;
    const double oldZ = entity->posZ;
    const int_t oldDim = entity->dimension;
    const int_t oldCooldown = entity->timeUntilPortal;
    const int_t oldFire = entity->fire;
    const float oldFall = entity->fallDistance;

    entity->posX = destX;
    entity->posY = destY;
    entity->posZ = destZ;
    entity->dimension = targetDim;
    entity->timeUntilPortal = entity->getPortalCooldown();
    entity->fire = 0;
    entity->fallDistance = 0.0f;

    NBTTagCompound *entityTag = new NBTTagCompound();
    const bool ok = entity->addEntityID(entityTag);

    // Restore live entity coordinates
    entity->posX = oldX;
    entity->posY = oldY;
    entity->posZ = oldZ;
    entity->dimension = oldDim;
    entity->timeUntilPortal = oldCooldown;
    entity->fire = oldFire;
    entity->fallDistance = oldFall;

    if (!ok)
    {
        delete entityTag;
        return false;
    }

    entitiesList->appendTag(entityTag);

    std::vector<byte_t> outBytes;
    VectorOutputStream outStream(outBytes);
    CompressedStreamTools::writeCompound(root.get(), outStream);

    writeRegion->write(chunkX & 31, chunkZ & 31, outBytes.data(), (int_t)outBytes.size());
    return true;
}

bool PortalTransferManager::queueEntityInDimension(const std::string &dimDir,
                                                  int_t chunkX, int_t chunkZ,
                                                  double destX, double destY, double destZ,
                                                  bool needsPortal, Entity *entity)
{
    const std::string queuePath = PlatformStorage::join(dimDir, "portal_queue.dat");

    NBTTagCompound *root = nullptr;
    if (PlatformStorage::exists(queuePath))
    {
        std::vector<unsigned char> bytes;
        if (PlatformStorage::readFile(queuePath, bytes) && !bytes.empty())
        {
            VectorInputStream vis(bytes);
            root = CompressedStreamTools::readCompound(vis);
        }
    }

    if (root == nullptr)
        root = new NBTTagCompound();

    NBTTagList *queueList = root->getTagList("Queue");
    if (queueList == nullptr)
    {
        queueList = new NBTTagList();
        root->setTag("Queue", queueList);
    }

    NBTTagCompound *entry = new NBTTagCompound();
    entry->setInteger("ChunkX", chunkX);
    entry->setInteger("ChunkZ", chunkZ);
    entry->setBoolean("NeedsPortal", needsPortal);
    entry->setDouble("TargetX", destX);
    entry->setDouble("TargetY", destY);
    entry->setDouble("TargetZ", destZ);

    const double oldX = entity->posX;
    const double oldY = entity->posY;
    const double oldZ = entity->posZ;
    const int_t oldDim = entity->dimension;
    const int_t oldCooldown = entity->timeUntilPortal;
    const int_t oldFire = entity->fire;
    const float oldFall = entity->fallDistance;

    const int_t targetDim = (dimDir.find("DIM-1") != std::string::npos) ? -1 :
                            ((dimDir.find("DIM1") != std::string::npos) ? 1 : 0);

    entity->posX = destX;
    entity->posY = destY;
    entity->posZ = destZ;
    entity->dimension = targetDim;
    entity->timeUntilPortal = entity->getPortalCooldown();
    entity->fire = 0;
    entity->fallDistance = 0.0f;

    NBTTagCompound *entityTag = new NBTTagCompound();
    const bool ok = entity->addEntityID(entityTag);

    entity->posX = oldX;
    entity->posY = oldY;
    entity->posZ = oldZ;
    entity->dimension = oldDim;
    entity->timeUntilPortal = oldCooldown;
    entity->fire = oldFire;
    entity->fallDistance = oldFall;

    if (!ok)
    {
        delete entityTag;
        delete entry;
        delete root;
        return false;
    }

    entry->setCompoundTag("Entity", entityTag);
    queueList->appendTag(entry);

    std::vector<byte_t> outBytes;
    VectorOutputStream vos(outBytes);
    CompressedStreamTools::writeCompound(root, vos);
    delete root;

    const std::string tmpPath = queuePath + ".tmp";
    if (PlatformStorage::writeFile(tmpPath, outBytes.data(), outBytes.size()))
    {
        PlatformStorage::renameFile(tmpPath, queuePath);
        return true;
    }
    return false;
}

bool PortalTransferManager::transferEntity(Entity *entity, int_t targetDim)
{
    if (entity == nullptr || entity->worldObj == nullptr || entity->worldObj->multiplayerWorld || entity->isDead)
        return false;
    if (entity->isPlayer() || entity->ridingEntity != nullptr || entity->riddenByEntity != nullptr)
        return false;
    if (!entity->canChangeDimensions())
        return false;

    World *world = entity->worldObj;
    ISaveHandler *saveHandler = world->getSaveHandler();
    if (saveHandler == nullptr)
        return false;

    const std::string worldDir = saveHandler->getSaveDirectory();
    const std::string targetDimDir = getDimensionDirectory(worldDir, targetDim);
    PlatformStorage::mkdirs(targetDimDir);

    const int_t currentDim = entity->dimension;
    double coordScale = 1.0;
    if (currentDim > -1 && targetDim == -1)
        coordScale = 0.125;
    else if (currentDim == -1 && targetDim > -1)
        coordScale = 8.0;

    double targetX = entity->posX * coordScale;
    double targetY = entity->posY;
    double targetZ = entity->posZ * coordScale;
    targetX = clampDouble(targetX, -29999872.0, 29999872.0);
    targetZ = clampDouble(targetZ, -29999872.0, 29999872.0);

    double destX = targetX;
    double destY = targetY;
    double destZ = targetZ;

    bool foundPortal = false;
    if (targetDim != 1)
    {
        foundPortal = findExistingPortalInDimension(targetDimDir, targetX, targetY, targetZ, destX, destY, destZ);
    }
    else
    {
        // End platform
        destX = 100.0;
        destY = 50.0;
        destZ = 0.0;
        foundPortal = true;
    }

    const int_t destChunkX = MathHelper::floor_double(destX) >> 4;
    const int_t destChunkZ = MathHelper::floor_double(destZ) >> 4;

    bool saved = false;
    if (foundPortal)
    {
        saved = writeEntityToDimensionChunk(targetDimDir, destChunkX, destChunkZ, destX, destY, destZ, targetDim, entity);
    }

    if (!saved)
    {
        saved = queueEntityInDimension(targetDimDir, destChunkX, destChunkZ, destX, destY, destZ, !foundPortal, entity);
    }

    if (saved)
    {
        MC_LOG_INFO("portal", "Entity %s transferred to dim %d at (%.1f, %.1f, %.1f) [chunk %d,%d, foundPortal=%d]\n",
                    EntityList::getEntityString(entity).c_str(), (int)targetDim, destX, destY, destZ, (int)destChunkX, (int)destChunkZ, (int)foundPortal);
        world->setEntityDead(entity);
        return true;
    }

    return false;
}

bool PortalTransferManager::processQueueForChunk(World *world, int_t chunkX, int_t chunkZ)
{
    if (world == nullptr || world->getSaveHandler() == nullptr || world->multiplayerWorld)
        return false;

    const std::string worldDir = world->getSaveHandler()->getSaveDirectory();
    const int_t dim = (world->worldProvider != nullptr) ? world->worldProvider->worldType : 0;
    const std::string dimDir = getDimensionDirectory(worldDir, dim);
    const std::string queuePath = PlatformStorage::join(dimDir, "portal_queue.dat");

    if (!PlatformStorage::exists(queuePath))
        return false;

    std::vector<unsigned char> data;
    if (!PlatformStorage::readFile(queuePath, data) || data.empty())
        return false;

    VectorInputStream vis(data);
    std::unique_ptr<NBTTagCompound> root(CompressedStreamTools::readCompound(vis));
    if (root == nullptr)
        return false;

    NBTTagList *queueList = root->getTagList("Queue");
    if (queueList == nullptr || queueList->tagCount() == 0)
    {
        PlatformStorage::removeFile(queuePath);
        return false;
    }

    NBTTagList *remainingList = new NBTTagList();
    int_t spawnedCount = 0;

    for (int_t i = 0; i < queueList->tagCount(); ++i)
    {
        NBTTagCompound *entry = dynamic_cast<NBTTagCompound *>(queueList->tagAt(i));
        if (entry == nullptr)
            continue;

        const int_t entryChunkX = entry->getInteger("ChunkX");
        const int_t entryChunkZ = entry->getInteger("ChunkZ");

        if (entryChunkX == chunkX && entryChunkZ == chunkZ)
        {
            NBTTagCompound *entityTag = entry->getCompoundTag("Entity");
            if (entityTag != nullptr)
            {
                Entity *spawned = EntityList::createEntityFromNBT(entityTag, world);
                if (spawned != nullptr)
                {
                    spawned->dimension = dim;
                    spawned->timeUntilPortal = spawned->getPortalCooldown();
                    world->spawnEntityInWorld(spawned);
                    spawnedCount++;
                    MC_LOG_INFO("portal", "Spawned queued entity %s for chunk (%d,%d)\n",
                                EntityList::getEntityString(spawned).c_str(), (int)chunkX, (int)chunkZ);
                    continue;
                }
            }
        }

        remainingList->appendTag(dynamic_cast<NBTBase *>(entry->copy()));
    }

    if (remainingList->tagCount() == 0)
    {
        delete remainingList;
        PlatformStorage::removeFile(queuePath);
    }
    else if (spawnedCount > 0)
    {
        root->setTag("Queue", remainingList);
        std::vector<byte_t> outBytes;
        VectorOutputStream vos(outBytes);
        CompressedStreamTools::writeCompound(root.get(), vos);

        const std::string tmpPath = queuePath + ".tmp";
        if (PlatformStorage::writeFile(tmpPath, outBytes.data(), outBytes.size()))
            PlatformStorage::renameFile(tmpPath, queuePath);
    }
    else
    {
        delete remainingList;
    }

    return spawnedCount > 0;
}

bool PortalTransferManager::processQueueForNewPortal(World *world, double portalX, double portalY, double portalZ)
{
    if (world == nullptr || world->getSaveHandler() == nullptr || world->multiplayerWorld)
        return false;

    const std::string worldDir = world->getSaveHandler()->getSaveDirectory();
    const int_t dim = (world->worldProvider != nullptr) ? world->worldProvider->worldType : 0;
    const std::string dimDir = getDimensionDirectory(worldDir, dim);
    const std::string queuePath = PlatformStorage::join(dimDir, "portal_queue.dat");

    if (!PlatformStorage::exists(queuePath))
        return false;

    std::vector<unsigned char> data;
    if (!PlatformStorage::readFile(queuePath, data) || data.empty())
        return false;

    VectorInputStream vis(data);
    std::unique_ptr<NBTTagCompound> root(CompressedStreamTools::readCompound(vis));
    if (root == nullptr)
        return false;

    NBTTagList *queueList = root->getTagList("Queue");
    if (queueList == nullptr || queueList->tagCount() == 0)
    {
        PlatformStorage::removeFile(queuePath);
        return false;
    }

    NBTTagList *remainingList = new NBTTagList();
    int_t spawnedCount = 0;

    for (int_t i = 0; i < queueList->tagCount(); ++i)
    {
        NBTTagCompound *entry = dynamic_cast<NBTTagCompound *>(queueList->tagAt(i));
        if (entry == nullptr)
            continue;

        const bool needsPortal = entry->getBoolean("NeedsPortal");
        const double targetX = entry->getDouble("TargetX");
        const double targetZ = entry->getDouble("TargetZ");

        const double dX = targetX - portalX;
        const double dZ = targetZ - portalZ;
        const double distSq = dX * dX + dZ * dZ;

        if (needsPortal || distSq <= (128.0 * 128.0))
        {
            NBTTagCompound *entityTag = entry->getCompoundTag("Entity");
            if (entityTag != nullptr)
            {
                Entity *spawned = EntityList::createEntityFromNBT(entityTag, world);
                if (spawned != nullptr)
                {
                    spawned->dimension = dim;
                    spawned->setLocationAndAngles(portalX, portalY, portalZ, spawned->rotationYaw, spawned->rotationPitch);
                    spawned->timeUntilPortal = spawned->getPortalCooldown();
                    world->spawnEntityInWorld(spawned);
                    spawnedCount++;
                    MC_LOG_INFO("portal", "Spawned queued entity %s at portal (%.1f, %.1f, %.1f)\n",
                                EntityList::getEntityString(spawned).c_str(), portalX, portalY, portalZ);
                    continue;
                }
            }
        }

        remainingList->appendTag(dynamic_cast<NBTBase *>(entry->copy()));
    }

    if (remainingList->tagCount() == 0)
    {
        delete remainingList;
        PlatformStorage::removeFile(queuePath);
    }
    else if (spawnedCount > 0)
    {
        root->setTag("Queue", remainingList);
        std::vector<byte_t> outBytes;
        VectorOutputStream vos(outBytes);
        CompressedStreamTools::writeCompound(root.get(), vos);

        const std::string tmpPath = queuePath + ".tmp";
        if (PlatformStorage::writeFile(tmpPath, outBytes.data(), outBytes.size()))
            PlatformStorage::renameFile(tmpPath, queuePath);
    }
    else
    {
        delete remainingList;
    }

    return spawnedCount > 0;
}
