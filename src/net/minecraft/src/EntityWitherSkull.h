#pragma once

#include "EntityFireball.h"

class World;
class EntityLiving;
class Explosion;
class Block;
class MovingObjectPosition;

// net.minecraft.src.EntityWitherSkull (Java 1.5.2): black skull, or blue "invulnerable" one that
// is slower and breaks even obsidian.
class EntityWitherSkull : public EntityFireball
{
public:
	explicit EntityWitherSkull(World *world);
	EntityWitherSkull(World *world, EntityLiving *shooter, double xAcceleration, double yAcceleration, double zAcceleration);
	EntityWitherSkull(World *world, double x, double y, double z, double xAcceleration, double yAcceleration, double zAcceleration);

	bool isBurning() override;
	float func_82146_a(Explosion *explosion, World *world, int_t x, int_t y, int_t z, Block *block) override;
	bool canBeCollidedWith() override;
	bool attackEntityFrom(Entity *entity, int_t damage) override;
	bool attackEntityFrom(const DamageSource &source, int_t damage) override;

	bool isInvulnerable();
	void setInvulnerable(bool value);

protected:
	void entityInit() override;
	float getMotionFactor() override;
	void onImpact(MovingObjectPosition *hit) override;
};
