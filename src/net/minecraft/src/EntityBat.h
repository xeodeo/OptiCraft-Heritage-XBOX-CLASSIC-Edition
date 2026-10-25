#pragma once

#include "EntityAmbientCreature.h"
#include "ChunkCoordinates.h"

class World;
class NBTTagCompound;
class DamageSource;

// net.minecraft.src.EntityBat
class EntityBat : public EntityAmbientCreature
{
public:
	explicit EntityBat(World *world);

	void entityInit() override;
	int_t getMaxHealth() const override;

	bool canBePushed() override;
	bool doesEntityNotTriggerPressurePlate() override;
	bool isAIEnabled() override;

	void onUpdate() override;
	void updateAITasks() override;

	bool attackEntityFrom(const DamageSource &source, int_t damage) override;
	void readEntityFromNBT(NBTTagCompound *nbttagcompound) override;
	void writeEntityToNBT(NBTTagCompound *nbttagcompound) override;
	bool getCanSpawnHere() override;
	void initCreature() override;

	bool getIsBatHanging() const;
	void setIsBatHanging(bool hanging);

	const char *getEntityTexture() override;

protected:
	float getSoundVolume() override;
	float getSoundPitch() override;
	jstring getLivingSound() override;
	jstring getHurtSound() override;
	jstring getDeathSound() override;

	void applyEntityCollision(Entity *entity) override;
	bool canTriggerWalking() override;
	void fall(float f) override;
	void updateFallState(double d, bool b) override;

private:
	ChunkCoordinates currentFlightTarget;
	bool hasFlightTarget = false;
};
