#pragma once

#include "EntityAIBase.h"
#include "java/Type.h"

class EntityLiving;
class IRangedAttackMob;
class World;

// net.minecraft.src.EntityAIArrowAttack
class EntityAIArrowAttack : public EntityAIBase
{
public:
	// Legacy 1.2.5 constructor
	EntityAIArrowAttack(EntityLiving *host, float moveSpeed, int_t rangedAttackId, int_t maxAttackTime);

	// 1.5.2 constructors
	EntityAIArrowAttack(IRangedAttackMob *host, float speed, int_t attackTime, float maxDistance);
	EntityAIArrowAttack(IRangedAttackMob *host, float speed, int_t minAttackTime, int_t maxAttackTime, float maxDistance);

	bool shouldExecute() override;
	bool continueExecuting() override;
	void resetTask() override;
	void updateTask() override;

private:
	EntityLiving *resolveTarget() const;
	void doRangedAttack(EntityLiving *target, double distanceSq);

	World *worldObj;
	EntityLiving *entityHost;
	IRangedAttackMob *rangedAttackEntityHost;
	int_t targetEntityId;
	int_t rangedAttackTime;
	float moveSpeed;
	int_t seeTime;
	int_t rangedAttackId;
	int_t minRangedAttackTime;
	int_t maxRangedAttackTime;
	float maxAttackDistance;
	float maxAttackDistanceSq;
};
