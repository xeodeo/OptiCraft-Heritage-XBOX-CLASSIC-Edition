#pragma once

#include <set>
#include <string>

#include "java/Type.h"

class Scoreboard;

// net.minecraft.src.ScorePlayerTeam (Java 1.5.2)
class ScorePlayerTeam
{
public:
	ScorePlayerTeam(Scoreboard *board, const std::string &name);

	const std::string &getRegisteredName() const { return registeredName; }
	const std::string &getTeamName() const { return teamDisplayName; }
	void setTeamName(const std::string &displayName);

	std::set<std::string> &getMembershipCollection() { return membershipSet; }
	const std::set<std::string> &getMembershipCollection() const { return membershipSet; }

	const std::string &getColorPrefix() const { return namePrefix; }
	void setNamePrefix(const std::string &prefix);

	const std::string &getColorSuffix() const { return nameSuffix; }
	void setNameSuffix(const std::string &suffix);

	static std::string formatPlayerName(const ScorePlayerTeam *team, const std::string &playerName);

	bool getAllowFriendlyFire() const { return allowFriendlyFire; }
	void setAllowFriendlyFire(bool allow);

	bool getSeeFriendlyInvisiblesEnabled() const { return canSeeFriendlyInvisibles; }
	void setSeeFriendlyInvisiblesEnabled(bool canSee);

	int_t packFriendlyFlags() const;
	void unpackFriendlyFlags(int_t flags);

private:
	Scoreboard *theScoreboard;
	std::string registeredName;
	std::set<std::string> membershipSet;
	std::string teamDisplayName;
	std::string namePrefix;
	std::string nameSuffix;
	bool allowFriendlyFire;
	bool canSeeFriendlyInvisibles;
};
