#pragma once

#include <string>
#include <vector>

#include "java/Type.h"

class EntityPlayer;
class Scoreboard;
class ScoreObjective;

// net.minecraft.src.Score (Java 1.5.2)
class Score
{
public:
	Score(Scoreboard *board, ScoreObjective *objective, const std::string &playerName);

	void increaseScore(int_t amount);
	void decreaseScore(int_t amount);
	void incrementScore();
	int_t getScorePoints() const { return scorePoints; }
	void setScorePoints(int_t points);
	ScoreObjective *getObjective() const { return theScoreObjective; }
	const std::string &getPlayerName() const { return scorePlayerName; }
	Scoreboard *getScoreboard() const { return theScoreboard; }
	void updateFromCriteria(const std::vector<EntityPlayer *> &players);

	// Java 1.5.2 ScoreComparator ordering (ascending by points so sidebar draws highest at top)
	static bool compareScores(const Score *a, const Score *b);

private:
	Scoreboard *theScoreboard;
	ScoreObjective *theScoreObjective;
	std::string scorePlayerName;
	int_t scorePoints;
};
