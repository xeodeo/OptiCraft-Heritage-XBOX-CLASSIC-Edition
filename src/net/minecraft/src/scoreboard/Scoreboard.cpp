#include "Scoreboard.h"

#include <algorithm>

#include "Score.h"
#include "ScoreObjective.h"
#include "ScoreObjectiveCriteria.h"
#include "ScorePlayerTeam.h"
#include "ScoreboardSaveData.h"
#include "java/String.h"

Scoreboard::Scoreboard()
	: objectiveDisplaySlots{nullptr, nullptr, nullptr}
	, saveData(nullptr)
{
	ScoreObjectiveCriteria::initialize();
}

Scoreboard::~Scoreboard()
{
	for (auto &playerEntry : field_96544_c)
	{
		for (auto &scoreEntry : playerEntry.second)
			delete scoreEntry.second;
	}
	field_96544_c.clear();

	for (auto &objEntry : scoreObjectives)
		delete objEntry.second;
	scoreObjectives.clear();
	scoreObjectiveCriterias.clear();

	for (auto &teamEntry : teams)
		delete teamEntry.second;
	teams.clear();
	teamMemberships.clear();
}

void Scoreboard::markSaveDataDirty()
{
	if (saveData != nullptr)
		saveData->markDirty();
}

ScoreObjective *Scoreboard::getObjective(const std::string &name) const
{
	auto it = scoreObjectives.find(name);
	return it != scoreObjectives.end() ? it->second : nullptr;
}

ScoreObjective *Scoreboard::addScoreObjective(const std::string &name, ScoreObjectiveCriteria *criteria)
{
	if (criteria == nullptr || getObjective(name) != nullptr)
		return nullptr;

	ScoreObjective *obj = new ScoreObjective(this, name, criteria);
	scoreObjectiveCriterias[criteria].push_back(obj);
	scoreObjectives[name] = obj;
	onObjectiveAdded(obj);
	return obj;
}

std::vector<ScoreObjective *> Scoreboard::getObjectivesByCriteria(ScoreObjectiveCriteria *criteria) const
{
	auto it = scoreObjectiveCriterias.find(criteria);
	if (it == scoreObjectiveCriterias.end())
		return {};
	return it->second;
}

Score *Scoreboard::getValueFromObjective(const std::string &playerName, ScoreObjective *objective)
{
	if (objective == nullptr)
		return nullptr;

	auto &playerMap = field_96544_c[playerName];
	auto it = playerMap.find(objective);
	if (it == playerMap.end())
	{
		Score *score = new Score(this, objective, playerName);
		playerMap[objective] = score;
		markSaveDataDirty();
		return score;
	}
	return it->second;
}

bool Scoreboard::hasPlayerScore(const std::string &playerName, ScoreObjective *objective) const
{
	if (objective == nullptr)
		return false;
	auto it = field_96544_c.find(playerName);
	if (it == field_96544_c.end())
		return false;
	return it->second.find(objective) != it->second.end();
}

std::vector<Score *> Scoreboard::getSortedScores(ScoreObjective *objective) const
{
	std::vector<Score *> result;
	if (objective == nullptr)
		return result;

	for (const auto &playerEntry : field_96544_c)
	{
		auto it = playerEntry.second.find(objective);
		if (it != playerEntry.second.end() && it->second != nullptr)
			result.push_back(it->second);
	}
	std::sort(result.begin(), result.end(), Score::compareScores);
	return result;
}

std::vector<ScoreObjective *> Scoreboard::getScoreObjectives() const
{
	std::vector<ScoreObjective *> result;
	result.reserve(scoreObjectives.size());
	for (const auto &entry : scoreObjectives)
		result.push_back(entry.second);
	return result;
}

std::vector<std::string> Scoreboard::getObjectiveNames() const
{
	std::vector<std::string> result;
	result.reserve(field_96544_c.size());
	for (const auto &entry : field_96544_c)
		result.push_back(entry.first);
	return result;
}

void Scoreboard::resetPlayerScores(const std::string &playerName)
{
	auto it = field_96544_c.find(playerName);
	if (it != field_96544_c.end())
	{
		for (auto &scoreEntry : it->second)
			delete scoreEntry.second;
		field_96544_c.erase(it);
		onPlayerScoresReset(playerName);
	}
}

std::vector<Score *> Scoreboard::getScores() const
{
	std::vector<Score *> result;
	for (const auto &playerEntry : field_96544_c)
	{
		for (const auto &scoreEntry : playerEntry.second)
		{
			if (scoreEntry.second != nullptr)
				result.push_back(scoreEntry.second);
		}
	}
	return result;
}

std::map<ScoreObjective *, Score *> Scoreboard::getPlayerScores(const std::string &playerName) const
{
	auto it = field_96544_c.find(playerName);
	if (it == field_96544_c.end())
		return {};
	return it->second;
}

void Scoreboard::removeObjective(ScoreObjective *objective)
{
	if (objective == nullptr)
		return;

	scoreObjectives.erase(objective->getName());
	for (int_t i = 0; i < 3; ++i)
	{
		if (objectiveDisplaySlots[i] == objective)
			setObjectiveInDisplaySlot(i, nullptr);
	}

	auto critIt = scoreObjectiveCriterias.find(objective->getCriteria());
	if (critIt != scoreObjectiveCriterias.end())
	{
		auto &vec = critIt->second;
		vec.erase(std::remove(vec.begin(), vec.end(), objective), vec.end());
	}

	for (auto &playerEntry : field_96544_c)
	{
		auto sIt = playerEntry.second.find(objective);
		if (sIt != playerEntry.second.end())
		{
			delete sIt->second;
			playerEntry.second.erase(sIt);
		}
	}

	onObjectiveRemoved(objective);
	delete objective;
}

void Scoreboard::setObjectiveInDisplaySlot(int_t slot, ScoreObjective *objective)
{
	if (slot < 0 || slot >= 3)
		return;
	objectiveDisplaySlots[slot] = objective;
	markSaveDataDirty();
}

ScoreObjective *Scoreboard::getObjectiveInDisplaySlot(int_t slot) const
{
	if (slot < 0 || slot >= 3)
		return nullptr;
	return objectiveDisplaySlots[slot];
}

ScorePlayerTeam *Scoreboard::getTeam(const std::string &teamName) const
{
	auto it = teams.find(teamName);
	return it != teams.end() ? it->second : nullptr;
}

ScorePlayerTeam *Scoreboard::createTeam(const std::string &teamName)
{
	if (getTeam(teamName) != nullptr)
		return nullptr;
	ScorePlayerTeam *team = new ScorePlayerTeam(this, teamName);
	teams[teamName] = team;
	onTeamCreated(team);
	return team;
}

void Scoreboard::removeTeam(ScorePlayerTeam *team)
{
	if (team == nullptr)
		return;
	teams.erase(team->getRegisteredName());
	for (const std::string &member : team->getMembershipCollection())
		teamMemberships.erase(member);
	onTeamRemoved(team);
	delete team;
}

void Scoreboard::addPlayerToTeam(const std::string &playerName, ScorePlayerTeam *team)
{
	if (team == nullptr)
		return;
	if (getPlayersTeam(playerName) != nullptr)
		removePlayerFromTeams(playerName);
	teamMemberships[playerName] = team;
	team->getMembershipCollection().insert(playerName);
	markSaveDataDirty();
}

bool Scoreboard::removePlayerFromTeams(const std::string &playerName)
{
	ScorePlayerTeam *team = getPlayersTeam(playerName);
	if (team != nullptr)
	{
		removePlayerFromTeam(playerName, team);
		return true;
	}
	return false;
}

bool Scoreboard::removePlayerFromTeam(const std::string &playerName, ScorePlayerTeam *team)
{
	if (team == nullptr || getPlayersTeam(playerName) != team)
		return false;
	teamMemberships.erase(playerName);
	team->getMembershipCollection().erase(playerName);
	markSaveDataDirty();
	return true;
}

std::vector<std::string> Scoreboard::getTeamNames() const
{
	std::vector<std::string> result;
	result.reserve(teams.size());
	for (const auto &entry : teams)
		result.push_back(entry.first);
	return result;
}

std::vector<ScorePlayerTeam *> Scoreboard::getTeams() const
{
	std::vector<ScorePlayerTeam *> result;
	result.reserve(teams.size());
	for (const auto &entry : teams)
		result.push_back(entry.second);
	return result;
}

ScorePlayerTeam *Scoreboard::getPlayersTeam(const std::string &playerName) const
{
	auto it = teamMemberships.find(playerName);
	return it != teamMemberships.end() ? it->second : nullptr;
}

void Scoreboard::onObjectiveAdded(ScoreObjective *) { markSaveDataDirty(); }
void Scoreboard::onObjectiveDisplayNameChanged(ScoreObjective *) { markSaveDataDirty(); }
void Scoreboard::onObjectiveRemoved(ScoreObjective *) { markSaveDataDirty(); }
void Scoreboard::onScoreUpdated(Score *) { markSaveDataDirty(); }
void Scoreboard::onPlayerScoresReset(const std::string &) { markSaveDataDirty(); }
void Scoreboard::onTeamCreated(ScorePlayerTeam *) { markSaveDataDirty(); }
void Scoreboard::onTeamUpdated(ScorePlayerTeam *) { markSaveDataDirty(); }
void Scoreboard::onTeamRemoved(ScorePlayerTeam *) { markSaveDataDirty(); }

std::string Scoreboard::getObjectiveDisplaySlot(int_t slot)
{
	switch (slot)
	{
	case 0: return "list";
	case 1: return "sidebar";
	case 2: return "belowName";
	default: return "";
	}
}

int_t Scoreboard::getObjectiveDisplaySlotNumber(const std::string &slotName)
{
	if (String::equalsIgnoreCaseJava(slotName, "list"))
		return 0;
	if (String::equalsIgnoreCaseJava(slotName, "sidebar"))
		return 1;
	if (String::equalsIgnoreCaseJava(slotName, "belowName"))
		return 2;
	return -1;
}
