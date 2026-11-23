#include "ScoreObjective.h"
#include "Scoreboard.h"

ScoreObjective::ScoreObjective(Scoreboard *board, const std::string &objectiveName, ScoreObjectiveCriteria *criteria)
	: theScoreboard(board)
	, name(objectiveName)
	, objectiveCriteria(criteria)
	, displayName(objectiveName)
{
}

void ScoreObjective::setDisplayName(const std::string &newName)
{
	displayName = newName;
	if (theScoreboard != nullptr)
		theScoreboard->onObjectiveDisplayNameChanged(this);
}
