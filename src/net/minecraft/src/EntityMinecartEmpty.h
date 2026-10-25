#pragma once
#include "EntityMinecart.h"
class EntityMinecartEmpty : public EntityMinecart { public: EntityMinecartEmpty(World *w); EntityMinecartEmpty(World *w,double x,double y,double z); int_t getMinecartType() const override { return 0; } };
