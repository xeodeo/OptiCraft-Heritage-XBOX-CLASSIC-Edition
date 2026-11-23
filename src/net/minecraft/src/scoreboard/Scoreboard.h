#pragma once

#include <map>
#include <string>
#include <vector>

#include "java/Type.h"

class Score;
class ScoreObjective;
class ScoreObjectiveCriteria;
class ScorePlayerTeam;
class ScoreboardSaveData;

// net.minecraft.src.Scoreboard (Java 1.5.2)
class Scoreboard
{
public:
	static constexpr int_t DISPLAY_SLOT_LIST = 0;
	static constexpr int_t DISPLAY_SLOT_SIDEBAR = 1;
	static constexpr int_t DISPLAY_SLOT_BELOW_NAME = 2;

	Scoreboard();
	~Scoreboard();

	ScoreObjective *getObjective(const std::string &name) const;
	ScoreObjective *addScoreObjective(const std::string &name, ScoreObjectiveCriteria *criteria);
	std::vector<ScoreObjective *> getObjectivesByCriteria(ScoreObjectiveCriteria *criteria) const;

	Score *getValueFromObjective(const std::string &playerName, ScoreObjective *objective);
	bool hasPlayerScore(const std::string &playerName, ScoreObjective *objective) const;
	std::vector<Score *> getSortedScores(ScoreObjective *objective) const;
	std::vector<ScoreObjective *> getScoreObjectives() const;
	std::vector<std::string> getObjectiveNames() const;
	void resetPlayerScores(const std::string &playerName);
	std::vector<Score *> getScores() const;
	std::map<ScoreObjective *, Score *> getPlayerScores(const std::string &playerName) const;

	void removeObjective(ScoreObjective *objective);
	void setObjectiveInDisplaySlot(int_t slot, ScoreObjective *objective);
	ScoreObjective *getObjectiveInDisplaySlot(int_t slot) const;

	ScorePlayerTeam *getTeam(const std::string &teamName) const;
	ScorePlayerTeam *createTeam(const std::string &teamName);
	void removeTeam(ScorePlayerTeam *team);
	void addPlayerToTeam(const std::string &playerName, ScorePlayerTeam *team);
	bool removePlayerFromTeams(const std::string &playerName);
	bool removePlayerFromTeam(const std::string &playerName, ScorePlayerTeam *team);
	std::vector<std::string> getTeamNames() const;
	std::vector<ScorePlayerTeam *> getTeams() const;
	ScorePlayerTeam *getPlayersTeam(const std::string &playerName) const;

	// Save-data dirty hook
	void setSaveData(ScoreboardSaveData *data) { saveData = data; }
	void markSaveDataDirty();

	// Change notifications (mark dirty)
	void onObjectiveAdded(ScoreObjective *objective);
	void onObjectiveDisplayNameChanged(ScoreObjective *objective);
	void onObjectiveRemoved(ScoreObjective *objective);
	void onScoreUpdated(Score *score);
	void onPlayerScoresReset(const std::string &playerName);
	void onTeamCreated(ScorePlayerTeam *team);
	void onTeamUpdated(ScorePlayerTeam *team);
	void onTeamRemoved(ScorePlayerTeam *team);

	static std::string getObjectiveDisplaySlot(int_t slot);
	static int_t getObjectiveDisplaySlotNumber(const std::string &slotName);

private:
	std::map<std::string, ScoreObjective *> scoreObjectives;
	std::map<ScoreObjectiveCriteria *, std::vector<ScoreObjective *>> scoreObjectiveCriterias;
	std::map<std::string, std::map<ScoreObjective *, Score *>> field_96544_c;
	ScoreObjective *objectiveDisplaySlots[3];
	std::map<std::string, ScorePlayerTeam *> teams;
	std::map<std::string, ScorePlayerTeam *> teamMemberships;
	ScoreboardSaveData *saveData;
};
