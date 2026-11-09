#pragma once

#include <typeinfo>

#include "EntityAITarget.h"

class Entity;
class EntityLiving;

// net.minecraft.src.EntityAINearestAttackableTarget
class EntityAINearestAttackableTarget : public EntityAITarget
{
public:
	EntityAINearestAttackableTarget(EntityLiving *owner, const std::type_info &targetType, float targetDistance,
		int_t targetChance, bool shouldCheckSight);
	EntityAINearestAttackableTarget(EntityLiving *owner, const std::type_info &targetType, float targetDistance,
		int_t targetChance, bool shouldCheckSight, bool nearbyOnly);
	// Java's IEntitySelector: only candidates the selector accepts.
	using EntitySelector = bool (*)(Entity *);
	EntityAINearestAttackableTarget(EntityLiving *owner, const std::type_info &targetType, float targetDistance,
		int_t targetChance, bool shouldCheckSight, bool nearbyOnly, EntitySelector selector);

	bool shouldExecute() override;
	void startExecuting() override;

private:
	EntityLiving *resolveTarget() const;

	const std::type_info *targetClass;
	int_t targetChance;
	int_t targetEntityId;
	EntitySelector entitySelector = nullptr;
};
