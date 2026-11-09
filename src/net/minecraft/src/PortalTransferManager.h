#pragma once

#include "java/Type.h"
#include <string>

class Entity;
class World;

class PortalTransferManager
{
public:
    // Dimension directory helper
    static std::string getDimensionDirectory(const std::string &worldDir, int_t dimension);

    // Initiates dimension transfer for an entity in a portal.
    static bool transferEntity(Entity *entity, int_t targetDim);

    // Checks and spawns queued entities when a chunk is loaded in the active world.
    static bool processQueueForChunk(World *world, int_t chunkX, int_t chunkZ);

    // Checks and spawns queued entities when the player arrives at / creates a portal in the active world.
    static bool processQueueForNewPortal(World *world, double portalX, double portalY, double portalZ);

private:
    static bool findExistingPortalInDimension(const std::string &dimDir,
                                             double targetX, double targetY, double targetZ,
                                             double &outPortalX, double &outPortalY, double &outPortalZ);

    static bool writeEntityToDimensionChunk(const std::string &dimDir,
                                           int_t chunkX, int_t chunkZ,
                                           double destX, double destY, double destZ,
                                           int_t targetDim, Entity *entity);

    static bool queueEntityInDimension(const std::string &dimDir,
                                      int_t chunkX, int_t chunkZ,
                                      double destX, double destY, double destZ,
                                      bool needsPortal, Entity *entity);
};
