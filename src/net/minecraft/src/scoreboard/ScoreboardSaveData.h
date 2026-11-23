#pragma once

#include <string>

#include "net/minecraft/src/MapDataBase.h"

class NBTTagCompound;
class NBTTagList;
class Scoreboard;
class ScorePlayerTeam;

// net.minecraft.src.ScoreboardSaveData (Java 1.5.2, data/scoreboard.dat)
class ScoreboardSaveData : public MapDataBase
{
public:
	ScoreboardSaveData();
	explicit ScoreboardSaveData(const std::string &name);
	~ScoreboardSaveData() override;

	void setScoreboard(Scoreboard *scoreboard);

	void readFromNBT(NBTTagCompound *tag) override;
	void writeToNBT(NBTTagCompound *tag) override;

private:
	void readTeams(NBTTagList *list);
	void readTeamPlayers(ScorePlayerTeam *team, NBTTagList *list);
	void readDisplaySlots(NBTTagCompound *tag);
	void readObjectives(NBTTagList *list);
	void readScores(NBTTagList *list);

	NBTTagList *writeTeams() const;
	void writeDisplaySlots(NBTTagCompound *tag) const;
	NBTTagList *writeObjectives() const;
	NBTTagList *writeScores() const;

	Scoreboard *theScoreboard;
	NBTTagCompound *pendingTag;
};
