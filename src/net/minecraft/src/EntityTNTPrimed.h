#pragma once

#include "Entity.h"
#include "java/Type.h"

class World;
class NBTTagCompound;
class EntityLiving;

// net.minecraft.src.EntityTNTPrimed
class EntityTNTPrimed : public Entity
{
public:
	EntityTNTPrimed(World *world);
	EntityTNTPrimed(World *world, double d, double d1, double d2);
	EntityTNTPrimed(World *world, double d, double d1, double d2, EntityLiving *placedBy);

protected:
	void entityInit() override;
	bool canTriggerWalking() override;

public:
	bool canBeCollidedWith() override;
	void onUpdate() override;
	void writeEntityToNBT(NBTTagCompound *nbttagcompound) override;
	void readEntityFromNBT(NBTTagCompound *nbttagcompound) override;
	float getShadowSize() override;
	EntityLiving *getTntPlacedBy() const;

private:
	void explode();

public:
	int_t fuse;

private:
	int_t tntPlacedByEntityId;
};
