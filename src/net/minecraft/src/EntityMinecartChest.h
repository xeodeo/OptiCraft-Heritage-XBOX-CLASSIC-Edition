#pragma once
#include "EntityMinecart.h"
class EntityMinecartChest : public EntityMinecart { public: EntityMinecartChest(World *w); EntityMinecartChest(World *w,double x,double y,double z); int_t getMinecartType() const override { return 1; } };
