#include "ScoreboardSaveData.h"

#include "Score.h"
#include "ScoreObjective.h"
#include "ScoreObjectiveCriteria.h"
#include "ScorePlayerTeam.h"
#include "Scoreboard.h"
#include "net/minecraft/src/NBTTagCompound.h"
#include "net/minecraft/src/NBTTagList.h"
#include "net/minecraft/src/NBTTagString.h"

ScoreboardSaveData::ScoreboardSaveData()
	: ScoreboardSaveData("scoreboard")
{
}

ScoreboardSaveData::ScoreboardSaveData(const std::string &name)
	: MapDataBase(name)
	, theScoreboard(nullptr)
	, pendingTag(nullptr)
{
}

ScoreboardSaveData::~ScoreboardSaveData()
{
	delete pendingTag;
	pendingTag = nullptr;
}

void ScoreboardSaveData::setScoreboard(Scoreboard *scoreboard)
{
	theScoreboard = scoreboard;
	if (theScoreboard != nullptr)
		theScoreboard->setSaveData(this);
	if (theScoreboard != nullptr && pendingTag != nullptr)
	{
		NBTTagCompound *tag = pendingTag;
		pendingTag = nullptr;
		readFromNBT(tag);
		delete tag;
		setDirty(false);
	}
}

void ScoreboardSaveData::readFromNBT(NBTTagCompound *tag)
{
	if (tag == nullptr)
		return;
	if (theScoreboard == nullptr)
	{
		delete pendingTag;
		pendingTag = static_cast<NBTTagCompound *>(tag->copy());
		return;
	}

	if (tag->hasKey("Objectives"))
		readObjectives(tag->getTagList("Objectives"));
	if (tag->hasKey("PlayerScores"))
		readScores(tag->getTagList("PlayerScores"));
	if (tag->hasKey("DisplaySlots"))
		readDisplaySlots(tag->getCompoundTag("DisplaySlots"));
	if (tag->hasKey("Teams"))
		readTeams(tag->getTagList("Teams"));
}

void ScoreboardSaveData::readTeams(NBTTagList *list)
{
	if (list == nullptr || theScoreboard == nullptr)
		return;
	for (int_t i = 0; i < list->tagCount(); ++i)
	{
		NBTTagCompound *teamTag = dynamic_cast<NBTTagCompound *>(list->tagAt(i));
		if (teamTag == nullptr)
			continue;
		const std::string name = teamTag->getString("Name");
		if (name.empty())
			continue;
		ScorePlayerTeam *team = theScoreboard->getTeam(name);
		if (team == nullptr)
			team = theScoreboard->createTeam(name);
		if (team == nullptr)
			continue;
		if (teamTag->hasKey("DisplayName"))
			team->setTeamName(teamTag->getString("DisplayName"));
		if (teamTag->hasKey("Prefix"))
			team->setNamePrefix(teamTag->getString("Prefix"));
		if (teamTag->hasKey("Suffix"))
			team->setNameSuffix(teamTag->getString("Suffix"));
		if (teamTag->hasKey("AllowFriendlyFire"))
			team->setAllowFriendlyFire(teamTag->getBoolean("AllowFriendlyFire"));
		if (teamTag->hasKey("SeeFriendlyInvisibles"))
			team->setSeeFriendlyInvisiblesEnabled(teamTag->getBoolean("SeeFriendlyInvisibles"));
		if (teamTag->hasKey("Players"))
			readTeamPlayers(team, teamTag->getTagList("Players"));
	}
}

void ScoreboardSaveData::readTeamPlayers(ScorePlayerTeam *team, NBTTagList *list)
{
	if (team == nullptr || list == nullptr || theScoreboard == nullptr)
		return;
	for (int_t i = 0; i < list->tagCount(); ++i)
	{
		NBTTagString *strTag = dynamic_cast<NBTTagString *>(list->tagAt(i));
		if (strTag != nullptr && !strTag->data.empty())
			theScoreboard->addPlayerToTeam(strTag->data, team);
	}
}

void ScoreboardSaveData::readDisplaySlots(NBTTagCompound *tag)
{
	if (tag == nullptr || theScoreboard == nullptr)
		return;
	for (int_t slot = 0; slot < 3; ++slot)
	{
		const std::string key = "slot_" + std::to_string(slot);
		if (tag->hasKey(key))
		{
			const std::string objName = tag->getString(key);
			ScoreObjective *obj = theScoreboard->getObjective(objName);
			theScoreboard->setObjectiveInDisplaySlot(slot, obj);
		}
	}
}

void ScoreboardSaveData::readObjectives(NBTTagList *list)
{
	if (list == nullptr || theScoreboard == nullptr)
		return;
	for (int_t i = 0; i < list->tagCount(); ++i)
	{
		NBTTagCompound *objTag = dynamic_cast<NBTTagCompound *>(list->tagAt(i));
		if (objTag == nullptr)
			continue;
		const std::string criteriaName = objTag->getString("CriteriaName");
		const std::string objName = objTag->getString("Name");
		ScoreObjectiveCriteria *criteria = ScoreObjectiveCriteria::getByName(criteriaName);
		if (criteria == nullptr || objName.empty())
			continue;
		ScoreObjective *obj = theScoreboard->getObjective(objName);
		if (obj == nullptr)
			obj = theScoreboard->addScoreObjective(objName, criteria);
		if (obj != nullptr && objTag->hasKey("DisplayName"))
			obj->setDisplayName(objTag->getString("DisplayName"));
	}
}

void ScoreboardSaveData::readScores(NBTTagList *list)
{
	if (list == nullptr || theScoreboard == nullptr)
		return;
	for (int_t i = 0; i < list->tagCount(); ++i)
	{
		NBTTagCompound *scoreTag = dynamic_cast<NBTTagCompound *>(list->tagAt(i));
		if (scoreTag == nullptr)
			continue;
		ScoreObjective *obj = theScoreboard->getObjective(scoreTag->getString("Objective"));
		if (obj == nullptr)
			continue;
		const std::string playerName = scoreTag->getString("Name");
		Score *score = theScoreboard->getValueFromObjective(playerName, obj);
		if (score != nullptr)
			score->setScorePoints(scoreTag->getInteger("Score"));
	}
}

void ScoreboardSaveData::writeToNBT(NBTTagCompound *tag)
{
	if (tag == nullptr || theScoreboard == nullptr)
		return;
	tag->setTag("Objectives", writeObjectives());
	tag->setTag("PlayerScores", writeScores());
	tag->setTag("Teams", writeTeams());
	writeDisplaySlots(tag);
}

NBTTagList *ScoreboardSaveData::writeTeams() const
{
	NBTTagList *list = new NBTTagList();
	for (ScorePlayerTeam *team : theScoreboard->getTeams())
	{
		if (team == nullptr)
			continue;
		NBTTagCompound *teamTag = new NBTTagCompound();
		teamTag->setString("Name", team->getRegisteredName());
		teamTag->setString("DisplayName", team->getTeamName());
		teamTag->setString("Prefix", team->getColorPrefix());
		teamTag->setString("Suffix", team->getColorSuffix());
		teamTag->setBoolean("AllowFriendlyFire", team->getAllowFriendlyFire());
		teamTag->setBoolean("SeeFriendlyInvisibles", team->getSeeFriendlyInvisiblesEnabled());

		NBTTagList *playersList = new NBTTagList();
		for (const std::string &member : team->getMembershipCollection())
			playersList->setTag(new NBTTagString(member));
		teamTag->setTag("Players", playersList);
		list->setTag(teamTag);
	}
	return list;
}

void ScoreboardSaveData::writeDisplaySlots(NBTTagCompound *tag) const
{
	NBTTagCompound *slotsTag = new NBTTagCompound();
	bool anySlot = false;
	for (int_t slot = 0; slot < 3; ++slot)
	{
		ScoreObjective *obj = theScoreboard->getObjectiveInDisplaySlot(slot);
		if (obj != nullptr)
		{
			slotsTag->setString("slot_" + std::to_string(slot), obj->getName());
			anySlot = true;
		}
	}
	if (anySlot)
		tag->setCompoundTag("DisplaySlots", slotsTag);
	else
		delete slotsTag;
}

NBTTagList *ScoreboardSaveData::writeObjectives() const
{
	NBTTagList *list = new NBTTagList();
	for (ScoreObjective *obj : theScoreboard->getScoreObjectives())
	{
		if (obj == nullptr || obj->getCriteria() == nullptr)
			continue;
		NBTTagCompound *objTag = new NBTTagCompound();
		objTag->setString("Name", obj->getName());
		objTag->setString("CriteriaName", obj->getCriteria()->getName());
		objTag->setString("DisplayName", obj->getDisplayName());
		list->setTag(objTag);
	}
	return list;
}

NBTTagList *ScoreboardSaveData::writeScores() const
{
	NBTTagList *list = new NBTTagList();
	for (Score *score : theScoreboard->getScores())
	{
		if (score == nullptr || score->getObjective() == nullptr)
			continue;
		NBTTagCompound *scoreTag = new NBTTagCompound();
		scoreTag->setString("Name", score->getPlayerName());
		scoreTag->setString("Objective", score->getObjective()->getName());
		scoreTag->setInteger("Score", score->getScorePoints());
		list->setTag(scoreTag);
	}
	return list;
}
