#pragma once

#include <string>

#include "DamageSource.h"
#include "java/Type.h"

// net.minecraft.src.CombatEntry (Java 1.5.2)
class CombatEntry
{
public:
	CombatEntry(const DamageSource &source, int_t time, int_t healthBefore, int_t damage,
	            const std::string &fallSuffix, float fallDist);

	const DamageSource &getDamageSrc() const { return damageSrc; }
	int_t getDamage() const { return damageAmount; }
	bool isLivingDamageSrc() const { return livingAttacker; }
	bool isPlayerDamageSrc() const { return playerAttacker; }
	int_t getAttackerEntityId() const { return attackerEntityId; }
	const std::string &getFallSuffix() const { return fallLocation; }
	const std::string &getAttackerName() const { return attackerName; }
	const std::string &getAttackerItemName() const { return attackerItemName; }
	bool hasAttackerName() const { return !attackerName.empty(); }
	bool hasAttackerItemName() const { return !attackerItemName.empty(); }
	float getFallDistance() const;

private:
	DamageSource damageSrc;
	int_t timeTicks;
	int_t damageAmount;
	int_t healthBeforeHit;
	std::string fallLocation;
	float fallDistance;
	bool livingAttacker;
	bool playerAttacker;
	int_t attackerEntityId;
	std::string attackerName;
	std::string attackerItemName;
};
