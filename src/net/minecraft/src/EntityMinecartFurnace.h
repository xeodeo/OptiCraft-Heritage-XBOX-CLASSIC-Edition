#pragma once
#include "EntityMinecart.h"
class EntityMinecartFurnace : public EntityMinecart { public: EntityMinecartFurnace(World *w); EntityMinecartFurnace(World *w,double x,double y,double z); int_t getMinecartType() const override { return 2; } };
