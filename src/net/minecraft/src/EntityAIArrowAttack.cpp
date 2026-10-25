#include "EntityAIArrowAttack.h"

#include <algorithm>

#include "EntityArrow.h"
#include "EntityLiving.h"
#include "EntityLookHelper.h"
#include "EntitySenses.h"
#include "EntitySnowball.h"
#include "IRangedAttackMob.h"
#include "MathHelper.h"
#include "PathNavigate.h"
#include "World.h"

EntityAIArrowAttack::EntityAIArrowAttack(EntityLiving *host, float speed, int_t attackId, int_t maxAttackTime)
	: worldObj(host != nullptr ? host->worldObj : nullptr), entityHost(host), rangedAttackEntityHost(nullptr),
	  targetEntityId(-1), rangedAttackTime(0), moveSpeed(speed), seeTime(0), rangedAttackId(attackId),
	  minRangedAttackTime(maxAttackTime), maxRangedAttackTime(maxAttackTime), maxAttackDistance(10.0f),
	  maxAttackDistanceSq(100.0f)
{
	setMutexBits(3);
}

EntityAIArrowAttack::EntityAIArrowAttack(IRangedAttackMob *host, float speed, int_t attackTime, float maxDistance)
	: EntityAIArrowAttack(host, speed, attackTime, attackTime, maxDistance)
{
}

EntityAIArrowAttack::EntityAIArrowAttack(IRangedAttackMob *host, float speed, int_t minAttackTime, int_t maxAttackTime, float maxDistance)
	: rangedAttackEntityHost(host), targetEntityId(-1), rangedAttackTime(-1), moveSpeed(speed), seeTime(0),
	  rangedAttackId(0), minRangedAttackTime(minAttackTime), maxRangedAttackTime(maxAttackTime),
	  maxAttackDistance(maxDistance), maxAttackDistanceSq(maxDistance * maxDistance)
{
	entityHost = dynamic_cast<EntityLiving *>(host);
	worldObj = entityHost != nullptr ? entityHost->worldObj : nullptr;
	setMutexBits(3);
}

EntityLiving *EntityAIArrowAttack::resolveTarget() const
{
	if (worldObj == nullptr || targetEntityId < 0)
		return nullptr;
	return dynamic_cast<EntityLiving *>(worldObj->getEntityByID(targetEntityId));
}

bool EntityAIArrowAttack::shouldExecute()
{
	if (entityHost == nullptr)
		return false;
	EntityLiving *target = entityHost->getAttackTarget();
	if (target == nullptr)
		return false;
	targetEntityId = target->entityId;
	return true;
}

bool EntityAIArrowAttack::continueExecuting()
{
	return shouldExecute() ||
		(entityHost != nullptr && entityHost->getNavigator() != nullptr && !entityHost->getNavigator()->noPath());
}

void EntityAIArrowAttack::resetTask()
{
	targetEntityId = -1;
	seeTime = 0;
	rangedAttackTime = -1;
}

void EntityAIArrowAttack::updateTask()
{
	EntityLiving *target = resolveTarget();
	if (entityHost == nullptr || target == nullptr || entityHost->getNavigator() == nullptr)
		return;
	double distanceSq = entityHost->getDistanceSq(target->posX, target->boundingBox->minY, target->posZ);
	EntitySenses *senses = entityHost->getEntitySenses();
	bool canSee = senses != nullptr && senses->canSee(target);
	if (canSee)
		++seeTime;
	else
		seeTime = 0;

	if (distanceSq <= static_cast<double>(maxAttackDistanceSq) && seeTime >= 20)
		entityHost->getNavigator()->clearPathEntity();
	else
		entityHost->getNavigator()->tryMoveToEntityLiving(target, moveSpeed);

	if (entityHost->getLookHelper() != nullptr)
		entityHost->getLookHelper()->setLookPositionWithEntity(target, 30.0f, 30.0f);

	if (rangedAttackEntityHost != nullptr)
	{
		if (--rangedAttackTime == 0)
		{
			if (distanceSq > static_cast<double>(maxAttackDistanceSq) || !canSee)
				return;

			float distFactor = MathHelper::sqrt_double(distanceSq) / maxAttackDistance;
			float factor = distFactor;
			if (factor < 0.1f)
				factor = 0.1f;
			if (factor > 1.0f)
				factor = 1.0f;

			rangedAttackEntityHost->attackEntityWithRangedAttack(target, factor);
			rangedAttackTime = MathHelper::floor_float(distFactor * static_cast<float>(maxRangedAttackTime - minRangedAttackTime) + static_cast<float>(minRangedAttackTime));
		}
		else if (rangedAttackTime < 0)
		{
			float distFactor = MathHelper::sqrt_double(distanceSq) / maxAttackDistance;
			rangedAttackTime = MathHelper::floor_float(distFactor * static_cast<float>(maxRangedAttackTime - minRangedAttackTime) + static_cast<float>(minRangedAttackTime));
		}
	}
	else
	{
		rangedAttackTime = std::max(rangedAttackTime - 1, 0);
		if (rangedAttackTime <= 0 && distanceSq <= static_cast<double>(maxAttackDistanceSq) && canSee)
		{
			doRangedAttack(target, distanceSq);
			rangedAttackTime = maxRangedAttackTime;
		}
	}
}

void EntityAIArrowAttack::doRangedAttack(EntityLiving *target, double distanceSq)
{
	(void)distanceSq;
	if (worldObj == nullptr || entityHost == nullptr || target == nullptr)
		return;
	if (rangedAttackId == 1)
	{
		EntityArrow *arrow = new EntityArrow(worldObj, entityHost, target, 1.6f, 12.0f);
		worldObj->playSoundAtEntity(entityHost, "random.bow", 1.0f, 1.0f / (entityHost->getRNG().nextFloat() * 0.4f + 0.8f));
		if (!worldObj->entityJoinedWorld(arrow))
			delete arrow;
	}
	else if (rangedAttackId == 2)
	{
		EntitySnowball *snowball = new EntitySnowball(worldObj, entityHost);
		double dx = target->posX - entityHost->posX;
		double dy = target->posY + static_cast<double>(target->getEyeHeight()) - static_cast<double>(1.1f) - snowball->posY;
		double dz = target->posZ - entityHost->posZ;
		float arc = MathHelper::sqrt_double(dx * dx + dz * dz) * 0.2f;
		snowball->setSnowballHeading(dx, dy + static_cast<double>(arc), dz, 1.6f, 12.0f);
		worldObj->playSoundAtEntity(entityHost, "random.bow", 1.0f, 1.0f / (entityHost->getRNG().nextFloat() * 0.4f + 0.8f));
		if (!worldObj->entityJoinedWorld(snowball))
			delete snowball;
	}
}
