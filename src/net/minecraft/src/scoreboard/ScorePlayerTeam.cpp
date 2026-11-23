#include "ScorePlayerTeam.h"

#include "Scoreboard.h"

ScorePlayerTeam::ScorePlayerTeam(Scoreboard *board, const std::string &name)
	: theScoreboard(board)
	, registeredName(name)
	, teamDisplayName(name)
	, namePrefix("")
	, nameSuffix("")
	, allowFriendlyFire(true)
	, canSeeFriendlyInvisibles(true)
{
}

void ScorePlayerTeam::setTeamName(const std::string &displayName)
{
	teamDisplayName = displayName;
	if (theScoreboard != nullptr)
		theScoreboard->onTeamUpdated(this);
}

void ScorePlayerTeam::setNamePrefix(const std::string &prefix)
{
	namePrefix = prefix;
	if (theScoreboard != nullptr)
		theScoreboard->onTeamUpdated(this);
}

void ScorePlayerTeam::setNameSuffix(const std::string &suffix)
{
	nameSuffix = suffix;
	if (theScoreboard != nullptr)
		theScoreboard->onTeamUpdated(this);
}

std::string ScorePlayerTeam::formatPlayerName(const ScorePlayerTeam *team, const std::string &playerName)
{
	if (team == nullptr)
		return playerName;
	return team->getColorPrefix() + playerName + team->getColorSuffix();
}

void ScorePlayerTeam::setAllowFriendlyFire(bool allow)
{
	allowFriendlyFire = allow;
	if (theScoreboard != nullptr)
		theScoreboard->onTeamUpdated(this);
}

void ScorePlayerTeam::setSeeFriendlyInvisiblesEnabled(bool canSee)
{
	canSeeFriendlyInvisibles = canSee;
	if (theScoreboard != nullptr)
		theScoreboard->onTeamUpdated(this);
}

int_t ScorePlayerTeam::packFriendlyFlags() const
{
	int_t flags = 0;
	if (allowFriendlyFire)
		flags |= 1;
	if (canSeeFriendlyInvisibles)
		flags |= 2;
	return flags;
}

void ScorePlayerTeam::unpackFriendlyFlags(int_t flags)
{
	setAllowFriendlyFire((flags & 1) != 0);
	setSeeFriendlyInvisiblesEnabled((flags & 2) != 0);
}
