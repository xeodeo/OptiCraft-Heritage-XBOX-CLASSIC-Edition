#pragma once

#include <map>
#include <string>
#include <vector>

#include "java/Type.h"

class EntityPlayer;

// net.minecraft.src.ScoreObjectiveCriteria (Java 1.5.2)
class ScoreObjectiveCriteria
{
public:
	virtual ~ScoreObjectiveCriteria() = default;

	virtual const std::string &getName() const = 0;
	virtual int_t computeScore(const std::vector<EntityPlayer *> &players) const = 0;
	virtual bool isReadOnly() const = 0;

	// MCP 1.5.2 aliases
	const std::string &func_96636_a() const { return getName(); }
	int_t func_96635_a(const std::vector<EntityPlayer *> &players) const { return computeScore(players); }

	static void initialize();
	static ScoreObjectiveCriteria *getByName(const std::string &name);
	static const std::map<std::string, ScoreObjectiveCriteria *> &getCriteriaMap();

	static ScoreObjectiveCriteria *DUMMY;
	static ScoreObjectiveCriteria *DEATH_COUNT;
	static ScoreObjectiveCriteria *PLAYER_KILL_COUNT;
	static ScoreObjectiveCriteria *TOTAL_KILL_COUNT;
	static ScoreObjectiveCriteria *HEALTH;
};

class ScoreDummyCriteria : public ScoreObjectiveCriteria
{
public:
	explicit ScoreDummyCriteria(const std::string &name);

	const std::string &getName() const override { return name; }
	int_t computeScore(const std::vector<EntityPlayer *> &players) const override
	{
		(void)players;
		return 0;
	}
	bool isReadOnly() const override { return false; }

private:
	std::string name;
};

class ScoreHealthCriteria : public ScoreDummyCriteria
{
public:
	explicit ScoreHealthCriteria(const std::string &name);

	int_t computeScore(const std::vector<EntityPlayer *> &players) const override;
	bool isReadOnly() const override { return true; }
};
