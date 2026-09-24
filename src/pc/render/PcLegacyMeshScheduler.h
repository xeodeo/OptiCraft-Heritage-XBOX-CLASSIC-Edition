#pragma once

#include "platform/PlatformConfig.h"

#if PLATFORM_INCREMENTAL_TERRAIN_BUILD

#include <vector>

#include "java/Type.h"

class EntityLiving;
class WorldRenderer;

void pcLegacyRunMeshScheduler(
    const std::vector<WorldRenderer *> &pending,
    EntityLiving *viewer,
    bool frustumOnly,
    int_t requestedUpdateLimit);

#endif
