#pragma once

#include <string>
#include <vector>

#include "CombatEntry.h"
#include "java/Type.h"

class DamageSource;
class EntityLiving;

// net.minecraft.src.CombatTracker (Java 1.5.2)
class CombatTracker
{
public:
	explicit CombatTracker(EntityLiving *fighter);

	void updateFallState();
	void recordDamage(const DamageSource &source, int_t healthBefore, int_t damage);
	std::string getDeathMessage();
	EntityLiving *getBestAttacker() const;
	void reset();

private:
	const CombatEntry *getBestCombatEntry() const;
	static std::string getFallSuffixOrGeneric(const CombatEntry *entry);
	void resetFallSuffix();
	void resetCombatIfExpired();

	std::vector<CombatEntry> combatEntries;
	EntityLiving *fighter;
	int_t lastDamageTime;
	bool inCombat;
	bool takingDamage;
	std::string fallSuffix;
};
