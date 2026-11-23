#include "Score.h"

#include "ScoreObjective.h"
#include "ScoreObjectiveCriteria.h"
#include "Scoreboard.h"

Score::Score(Scoreboard *board, ScoreObjective *objective, const std::string &playerName)
	: theScoreboard(board)
	, theScoreObjective(objective)
	, scorePlayerName(playerName)
	, scorePoints(0)
{
}

void Score::increaseScore(int_t amount)
{
	if (theScoreObjective != nullptr && theScoreObjective->getCriteria() != nullptr &&
	    theScoreObjective->getCriteria()->isReadOnly())
		return;
	setScorePoints(scorePoints + amount);
}

void Score::decreaseScore(int_t amount)
{
	if (theScoreObjective != nullptr && theScoreObjective->getCriteria() != nullptr &&
	    theScoreObjective->getCriteria()->isReadOnly())
		return;
	setScorePoints(scorePoints - amount);
}

void Score::incrementScore()
{
	increaseScore(1);
}

void Score::setScorePoints(int_t points)
{
	const int_t oldPoints = scorePoints;
	scorePoints = points;
	if (oldPoints != points && theScoreboard != nullptr)
		theScoreboard->onScoreUpdated(this);
}

void Score::updateFromCriteria(const std::vector<EntityPlayer *> &players)
{
	if (theScoreObjective != nullptr && theScoreObjective->getCriteria() != nullptr)
		setScorePoints(theScoreObjective->getCriteria()->computeScore(players));
}

bool Score::compareScores(const Score *a, const Score *b)
{
	if (a == nullptr || b == nullptr)
		return a != nullptr;
	if (a->scorePoints != b->scorePoints)
		return a->scorePoints < b->scorePoints;
	return a->scorePlayerName < b->scorePlayerName;
}
