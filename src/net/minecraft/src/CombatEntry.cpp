#include "CombatEntry.h"

#include <cfloat>

#include "Entity.h"
#include "EntityLiving.h"
#include "EntityPlayer.h"
#include "ItemStack.h"

CombatEntry::CombatEntry(const DamageSource &source, int_t time, int_t healthBefore, int_t damage,
                         const std::string &fallSuffix, float fallDist)
	: damageSrc(source)
	, timeTicks(time)
	, damageAmount(damage)
	, healthBeforeHit(healthBefore)
	, fallLocation(fallSuffix)
	, fallDistance(fallDist)
	, livingAttacker(false)
	, playerAttacker(false)
	, attackerEntityId(-1)
{
	Entity *attacker = source.getEntity();
	if (attacker != nullptr)
	{
		attackerEntityId = attacker->entityId;
		attackerName = attacker->getTranslatedEntityName();
		livingAttacker = attacker->isLiving();
		playerAttacker = attacker->isPlayer();
		if (livingAttacker)
		{
			EntityLiving *living = static_cast<EntityLiving *>(attacker);
			ItemStack *held = living->getHeldItem();
			if (held != nullptr && held->hasDisplayName())
				attackerItemName = held->getDisplayName();
		}
	}
}

float CombatEntry::getFallDistance() const
{
	return (damageSrc.getDamageType() == DamageSource::outOfWorld.getDamageType()) ? FLT_MAX : fallDistance;
}
