#include "ScoreObjectiveCriteria.h"

#include <cmath>

#include "net/minecraft/src/EntityPlayer.h"
#include "net/minecraft/src/MathHelper.h"

ScoreObjectiveCriteria *ScoreObjectiveCriteria::DUMMY = nullptr;
ScoreObjectiveCriteria *ScoreObjectiveCriteria::DEATH_COUNT = nullptr;
ScoreObjectiveCriteria *ScoreObjectiveCriteria::PLAYER_KILL_COUNT = nullptr;
ScoreObjectiveCriteria *ScoreObjectiveCriteria::TOTAL_KILL_COUNT = nullptr;
ScoreObjectiveCriteria *ScoreObjectiveCriteria::HEALTH = nullptr;

namespace
{
std::map<std::string, ScoreObjectiveCriteria *> &criteriaRegistry()
{
	static std::map<std::string, ScoreObjectiveCriteria *> s_map;
	return s_map;
}
} // namespace

ScoreDummyCriteria::ScoreDummyCriteria(const std::string &criteriaName)
	: name(criteriaName)
{
	criteriaRegistry()[name] = this;
}

ScoreHealthCriteria::ScoreHealthCriteria(const std::string &criteriaName)
	: ScoreDummyCriteria(criteriaName)
{
}

int_t ScoreHealthCriteria::computeScore(const std::vector<EntityPlayer *> &players) const
{
	float total = 0.0f;
	for (EntityPlayer *p : players)
	{
		if (p != nullptr)
			total += static_cast<float>(p->getHealth());
	}
	if (!players.empty())
		total /= static_cast<float>(players.size());
	return static_cast<int_t>(std::ceil(total));
}

void ScoreObjectiveCriteria::initialize()
{
	if (DUMMY != nullptr)
		return;
	static ScoreDummyCriteria s_dummy("dummy");
	static ScoreDummyCriteria s_deathCount("deathCount");
	static ScoreDummyCriteria s_playerKillCount("playerKillCount");
	static ScoreDummyCriteria s_totalKillCount("totalKillCount");
	static ScoreHealthCriteria s_health("health");

	DUMMY = &s_dummy;
	DEATH_COUNT = &s_deathCount;
	PLAYER_KILL_COUNT = &s_playerKillCount;
	TOTAL_KILL_COUNT = &s_totalKillCount;
	HEALTH = &s_health;
}

ScoreObjectiveCriteria *ScoreObjectiveCriteria::getByName(const std::string &name)
{
	initialize();
	auto &reg = criteriaRegistry();
	auto it = reg.find(name);
	return it != reg.end() ? it->second : nullptr;
}

const std::map<std::string, ScoreObjectiveCriteria *> &ScoreObjectiveCriteria::getCriteriaMap()
{
	initialize();
	return criteriaRegistry();
}
