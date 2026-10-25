#pragma once

#include "EntityLiving.h"

// net.minecraft.src.EntityAmbientCreature
class EntityAmbientCreature : public EntityLiving
{
public:
	explicit EntityAmbientCreature(World *world);

	bool isAmbientCreature() const override { return true; }
};
