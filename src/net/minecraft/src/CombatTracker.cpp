#include "CombatTracker.h"

#include "Block.h"
#include "BlockVine.h"
#include "DamageSource.h"
#include "Entity.h"
#include "EntityLiving.h"
#include "EntityPlayer.h"
#include "MathHelper.h"
#include "StatCollector.h"
#include "World.h"

CombatTracker::CombatTracker(EntityLiving *entity)
	: fighter(entity)
	, lastDamageTime(0)
	, inCombat(false)
	, takingDamage(false)
{
}

void CombatTracker::updateFallState()
{
	resetFallSuffix();
	if (fighter == nullptr || fighter->worldObj == nullptr)
		return;

	if (fighter->isOnLadder())
	{
		const int_t bx = MathHelper::floor_double(fighter->posX);
		const int_t by = MathHelper::floor_double(fighter->boundingBox->minY);
		const int_t bz = MathHelper::floor_double(fighter->posZ);
		const int_t blockId = fighter->worldObj->getBlockId(bx, by, bz);
		if (Block::ladder != nullptr && blockId == Block::ladder->blockID)
			fallSuffix = "ladder";
		else if (Block::vine != nullptr && blockId == Block::vine->blockID)
			fallSuffix = "vines";
	}
	else if (fighter->isInWater())
	{
		fallSuffix = "water";
	}
}

void CombatTracker::recordDamage(const DamageSource &source, int_t healthBefore, int_t damage)
{
	if (fighter == nullptr)
		return;
	resetCombatIfExpired();
	updateFallState();
	CombatEntry entry(source, fighter->ticksExisted, healthBefore, damage, fallSuffix, fighter->fallDistance);
	combatEntries.push_back(entry);
	lastDamageTime = fighter->ticksExisted;
	takingDamage = true;
	inCombat = inCombat || entry.isLivingDamageSrc();
}

std::string CombatTracker::getDeathMessage()
{
	if (fighter == nullptr)
		return "";

	const std::string victimName = fighter->getTranslatedEntityName();
	if (combatEntries.empty())
		return StatCollector::translateToLocalFormatted("death.attack.generic", victimName);

	const CombatEntry *bestEntry = getBestCombatEntry();
	const CombatEntry &lastEntry = combatEntries.back();
	const std::string &lastAttackerName = lastEntry.getAttackerName();

	if (bestEntry != nullptr && lastEntry.getDamageSrc().getDamageType() == DamageSource::fall.getDamageType())
	{
		const std::string &bestAttackerName = bestEntry->getAttackerName();
		const jstring &bestType = bestEntry->getDamageSrc().getDamageType();
		if (bestType == DamageSource::fall.getDamageType() || bestType == DamageSource::outOfWorld.getDamageType())
		{
			return StatCollector::translateToLocalFormatted(
				"death.fell.accident." + getFallSuffixOrGeneric(bestEntry), victimName);
		}
		if (!bestAttackerName.empty() && (lastAttackerName.empty() || bestAttackerName != lastAttackerName))
		{
			if (bestEntry->hasAttackerItemName())
			{
				return StatCollector::translateToLocalFormatted(
					"death.fell.assist.item", victimName, bestAttackerName, bestEntry->getAttackerItemName());
			}
			return StatCollector::translateToLocalFormatted("death.fell.assist", victimName, bestAttackerName);
		}
		if (!lastAttackerName.empty())
		{
			if (lastEntry.hasAttackerItemName())
			{
				return StatCollector::translateToLocalFormatted(
					"death.fell.finish.item", victimName, lastAttackerName, lastEntry.getAttackerItemName());
			}
			return StatCollector::translateToLocalFormatted("death.fell.finish", victimName, lastAttackerName);
		}
		return StatCollector::translateToLocalFormatted("death.fell.killer", victimName);
	}

	return lastEntry.getDamageSrc().getDeathMessage(fighter);
}

EntityLiving *CombatTracker::getBestAttacker() const
{
	if (fighter == nullptr || fighter->worldObj == nullptr)
		return nullptr;

	int_t bestLivingId = -1;
	int_t bestPlayerId = -1;
	int_t maxLivingDamage = 0;
	int_t maxPlayerDamage = 0;

	for (const CombatEntry &entry : combatEntries)
	{
		const int_t id = entry.getAttackerEntityId();
		if (id < 0)
			continue;
		if (entry.isPlayerDamageSrc() && (bestPlayerId < 0 || entry.getDamage() > maxPlayerDamage))
		{
			maxPlayerDamage = entry.getDamage();
			bestPlayerId = id;
		}
		if (entry.isLivingDamageSrc() && (bestLivingId < 0 || entry.getDamage() > maxLivingDamage))
		{
			maxLivingDamage = entry.getDamage();
			bestLivingId = id;
		}
	}

	const int_t chosenId = (bestPlayerId >= 0 && maxPlayerDamage >= maxLivingDamage / 3) ? bestPlayerId : bestLivingId;
	if (chosenId < 0)
		return nullptr;
	Entity *e = fighter->worldObj->getEntityByID(chosenId);
	return (e != nullptr && e->isLiving()) ? static_cast<EntityLiving *>(e) : nullptr;
}

void CombatTracker::reset()
{
	combatEntries.clear();
	inCombat = false;
	takingDamage = false;
	fallSuffix.clear();
}

const CombatEntry *CombatTracker::getBestCombatEntry() const
{
	const CombatEntry *bestFall = nullptr;
	const CombatEntry *bestDamage = nullptr;
	int_t bestDmgVal = 0;
	float maxFall = 0.0f;

	for (std::size_t i = 0; i < combatEntries.size(); ++i)
	{
		const CombatEntry &cur = combatEntries[i];
		const CombatEntry *prev = (i > 0) ? &combatEntries[i - 1] : nullptr;
		const jstring &type = cur.getDamageSrc().getDamageType();

		if ((type == DamageSource::fall.getDamageType() || type == DamageSource::outOfWorld.getDamageType()) &&
		    cur.getFallDistance() > 0.0f && (bestFall == nullptr || cur.getFallDistance() > maxFall))
		{
			bestFall = (i > 0) ? prev : &cur;
			maxFall = cur.getFallDistance();
		}

		if (!cur.getFallSuffix().empty() && (bestDamage == nullptr || cur.getDamage() > bestDmgVal))
		{
			bestDamage = &cur;
			bestDmgVal = cur.getDamage();
		}
	}

	if (maxFall > 5.0f && bestFall != nullptr)
		return bestFall;
	if (bestDmgVal > 5 && bestDamage != nullptr)
		return bestDamage;
	return nullptr;
}

std::string CombatTracker::getFallSuffixOrGeneric(const CombatEntry *entry)
{
	return (entry == nullptr || entry->getFallSuffix().empty()) ? std::string("generic") : entry->getFallSuffix();
}

void CombatTracker::resetFallSuffix()
{
	fallSuffix.clear();
}

void CombatTracker::resetCombatIfExpired()
{
	if (fighter == nullptr)
		return;
	const int_t timeout = inCombat ? 300 : 100;
	if (takingDamage && (fighter->ticksExisted - lastDamageTime) > timeout)
	{
		combatEntries.clear();
		takingDamage = false;
		inCombat = false;
	}
}
