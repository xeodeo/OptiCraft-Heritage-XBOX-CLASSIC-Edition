#pragma once

#include <string>

class Scoreboard;
class ScoreObjectiveCriteria;

// net.minecraft.src.ScoreObjective (Java 1.5.2)
class ScoreObjective
{
public:
	ScoreObjective(Scoreboard *board, const std::string &objectiveName, ScoreObjectiveCriteria *criteria);

	Scoreboard *getScoreboard() const { return theScoreboard; }
	const std::string &getName() const { return name; }
	ScoreObjectiveCriteria *getCriteria() const { return objectiveCriteria; }
	const std::string &getDisplayName() const { return displayName; }
	void setDisplayName(const std::string &newName);

private:
	Scoreboard *theScoreboard;
	std::string name;
	ScoreObjectiveCriteria *objectiveCriteria;
	std::string displayName;
};
