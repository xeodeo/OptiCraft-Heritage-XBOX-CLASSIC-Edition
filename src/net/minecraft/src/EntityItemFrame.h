#pragma once
#include "EntityPainting.h"
class EntityItemFrame : public EntityPainting {
public:
    explicit EntityItemFrame(World *world) : EntityPainting(world) {}
    EntityItemFrame(World *world, int_t x, int_t y, int_t z, int_t dir) : EntityPainting(world, x, y, z, dir) {}
};
