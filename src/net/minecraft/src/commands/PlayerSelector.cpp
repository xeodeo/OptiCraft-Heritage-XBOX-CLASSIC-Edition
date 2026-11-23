#include "PlayerSelector.h"

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <map>

#include "ChunkCoordinates.h"
#include "CommandBase.h"
#include "EntityPlayer.h"
#include "EnumGameType.h"
#include "ICommandSender.h"
#include "Minecraft.h"
#include "World.h"
#include "WorldInfo.h"
#include "scoreboard/Score.h"
#include "scoreboard/ScoreObjective.h"
#include "scoreboard/ScorePlayerTeam.h"
#include "scoreboard/Scoreboard.h"
#include "java/Random.h"
#include "java/String.h"

namespace
{
bool parseSelectorToken(const std::string &token, char &outType, std::string &outArgBody)
{
	outType = '\0';
	outArgBody.clear();
	if (token.size() < 2 || token[0] != '@')
		return false;
	const char t = token[1];
	if (t != 'p' && t != 'a' && t != 'r' && t != 'f')
		return false;
	if (token.size() == 2)
	{
		outType = t;
		return true;
	}
	if (token.size() < 4 || token[2] != '[' || token.back() != ']')
		return false;
	outType = t;
	outArgBody = token.substr(3, token.size() - 4);
	return true;
}

std::map<std::string, std::string> parseArgumentMap(const std::string &body)
{
	std::map<std::string, std::string> args;
	if (body.empty())
		return args;

	std::size_t pos = 0;
	int positionalIndex = 0;
	static const char *kPositionalKeys[4] = {"x", "y", "z", "r"};
	while (pos <= body.size())
	{
		std::size_t comma = body.find(',', pos);
		std::string part = (comma == std::string::npos) ? body.substr(pos) : body.substr(pos, comma - pos);
		const std::size_t eq = part.find('=');
		if (eq != std::string::npos)
		{
			const std::string key = part.substr(0, eq);
			const std::string val = part.substr(eq + 1);
			if (!key.empty())
				args[key] = val;
			positionalIndex = 4; // once key=value starts, stop positional mapping
		}
		else if (positionalIndex < 4)
		{
			if (!part.empty())
				args[kPositionalKeys[positionalIndex]] = part;
			++positionalIndex;
		}
		if (comma == std::string::npos)
			break;
		pos = comma + 1;
	}
	return args;
}

int parseIntDefault(const std::string &s, int def)
{
	if (s.empty())
		return def;
	char *endPtr = nullptr;
	const long v = std::strtol(s.c_str(), &endPtr, 10);
	if (endPtr == nullptr || *endPtr != '\0')
		return def;
	return static_cast<int>(v);
}

int effectivePlayerGameType(Minecraft *mc, EntityPlayer *p)
{
	if (p == nullptr)
		return EnumGameType::SURVIVAL;
	if (p->capabilities.isCreativeMode)
		return EnumGameType::CREATIVE;
	if (!p->capabilities.allowEdit)
		return EnumGameType::ADVENTURE;
	if (p->getPlayerGameType() != EnumGameType::NOT_SET)
		return p->getPlayerGameType();
	if (mc != nullptr && mc->theWorld != nullptr && mc->theWorld->getWorldInfo() != nullptr)
		return mc->theWorld->getWorldInfo()->getGameType();
	return EnumGameType::SURVIVAL;
}
} // namespace

EntityPlayer *PlayerSelector::matchOnePlayer(ICommandSender *sender, const std::string &token)
{
	std::vector<EntityPlayer *> players = matchPlayers(sender, token);
	return players.size() == 1 ? players[0] : nullptr;
}

std::string PlayerSelector::matchPlayersAsString(ICommandSender *sender, const std::string &token)
{
	std::vector<EntityPlayer *> players = matchPlayers(sender, token);
	if (players.empty())
		return "";
	std::vector<std::string> names;
	names.reserve(players.size());
	for (EntityPlayer *p : players)
		names.push_back(p->username);
	return CommandBase::joinNiceString(names);
}

std::vector<EntityPlayer *> PlayerSelector::matchPlayers(ICommandSender *sender, const std::string &token, bool *matchedSyntax)
{
	char selType = '\0';
	std::string argBody;
	if (!parseSelectorToken(token, selType, argBody))
	{
		if (matchedSyntax != nullptr)
			*matchedSyntax = false;
		return {};
	}
	if (matchedSyntax != nullptr)
		*matchedSyntax = true;
	if (selType == 'f')
		return {};

	const std::map<std::string, std::string> args = parseArgumentMap(argBody);
	int minRange = 0;
	int maxRange = 0;
	int minLevel = 0;
	int maxLevel = INT_MAX;
	int count = (selType == 'a') ? 0 : 1;
	int gameMode = EnumGameType::NOT_SET;
	ChunkCoordinates origin = sender != nullptr ? sender->getPlayerCoordinates() : ChunkCoordinates(0, 64, 0);
	std::string nameFilter;
	bool hasNameFilter = false;

	auto getArg = [&](const char *key, int &target) {
		auto it = args.find(key);
		if (it != args.end())
			target = parseIntDefault(it->second, target);
	};
	getArg("rm", minRange);
	getArg("r", maxRange);
	getArg("lm", minLevel);
	getArg("l", maxLevel);
	getArg("x", origin.x);
	getArg("y", origin.y);
	getArg("z", origin.z);
	getArg("m", gameMode);
	getArg("c", count);
	if (auto it = args.find("name"); it != args.end())
	{
		nameFilter = it->second;
		hasNameFilter = true;
	}

	bool invertName = false;
	if (hasNameFilter && !nameFilter.empty() && nameFilter[0] == '!')
	{
		invertName = true;
		nameFilter = nameFilter.substr(1);
	}

	std::string teamFilter;
	bool hasTeamFilter = false;
	bool invertTeam = false;
	if (auto it = args.find("team"); it != args.end())
	{
		teamFilter = it->second;
		hasTeamFilter = true;
		if (!teamFilter.empty() && teamFilter[0] == '!')
		{
			invertTeam = true;
			teamFilter = teamFilter.substr(1);
		}
	}

	// score_<obj>=max and score_<obj>_min=min
	std::map<std::string, int> scoreMaxFilters;
	std::map<std::string, int> scoreMinFilters;
	for (const auto &entry : args)
	{
		if (entry.first.rfind("score_", 0) == 0 && entry.first.size() > 6)
		{
			std::string rest = entry.first.substr(6);
			if (rest.size() > 4 && rest.compare(rest.size() - 4, 4, "_min") == 0)
				scoreMinFilters[rest.substr(0, rest.size() - 4)] = parseIntDefault(entry.second, 0);
			else
				scoreMaxFilters[rest] = parseIntDefault(entry.second, 0);
		}
	}

	Minecraft *mc = sender != nullptr ? sender->getMinecraft() : nullptr;
	Scoreboard *board = (mc != nullptr && mc->theWorld != nullptr) ? mc->theWorld->getScoreboard() : nullptr;
	std::vector<EntityPlayer *> all = CommandBase::getAllPlayers(mc);
	std::vector<EntityPlayer *> matched;

	const double minRangeSq = static_cast<double>(minRange) * static_cast<double>(minRange);
	const double maxRangeSq = static_cast<double>(maxRange) * static_cast<double>(maxRange);
	const std::string lowerFilter = String::toLowerCaseJava(nameFilter);

	auto distSq = [&](EntityPlayer *p) -> double {
		const double dx = p->posX - static_cast<double>(origin.x);
		const double dy = (p->boundingBox != nullptr ? p->boundingBox->minY : p->posY) - static_cast<double>(origin.y);
		const double dz = p->posZ - static_cast<double>(origin.z);
		return dx * dx + dy * dy + dz * dz;
	};

	for (EntityPlayer *p : all)
	{
		if (p == nullptr)
			continue;
		if (hasNameFilter)
		{
			const bool eq = (static_cast<std::string>(String::toLowerCaseJava(p->username)) == lowerFilter);
			if (invertName ? eq : !eq)
				continue;
		}
		if (hasTeamFilter)
		{
			ScorePlayerTeam *pt = (board != nullptr) ? board->getPlayersTeam(p->username) : nullptr;
			const std::string pTeam = (pt != nullptr) ? pt->getRegisteredName() : "";
			const bool eq = (pTeam == teamFilter);
			if (invertTeam ? eq : !eq)
				continue;
		}
		if (!scoreMinFilters.empty() || !scoreMaxFilters.empty())
		{
			if (board == nullptr)
				continue;
			bool scoreOk = true;
			for (const auto &sf : scoreMinFilters)
			{
				ScoreObjective *obj = board->getObjective(sf.first);
				if (obj == nullptr || board->getValueFromObjective(p->username, obj)->getScorePoints() < sf.second)
				{
					scoreOk = false;
					break;
				}
			}
			if (!scoreOk)
				continue;
			for (const auto &sf : scoreMaxFilters)
			{
				ScoreObjective *obj = board->getObjective(sf.first);
				if (obj == nullptr || board->getValueFromObjective(p->username, obj)->getScorePoints() > sf.second)
				{
					scoreOk = false;
					break;
				}
			}
			if (!scoreOk)
				continue;
		}
		if (gameMode != EnumGameType::NOT_SET && effectivePlayerGameType(mc, p) != gameMode)
			continue;
		if (p->experienceLevel < minLevel || p->experienceLevel > maxLevel)
			continue;
		if (minRange > 0 || maxRange > 0)
		{
			const double d2 = distSq(p);
			if (minRange > 0 && d2 < minRangeSq)
				continue;
			if (maxRange > 0 && d2 > maxRangeSq)
				continue;
		}
		matched.push_back(p);
	}

	if (selType == 'r')
	{
		static Random s_rand;
		for (std::size_t i = matched.size(); i > 1; --i)
		{
			const std::size_t j = static_cast<std::size_t>(s_rand.nextInt(static_cast<int_t>(i)));
			std::swap(matched[i - 1], matched[j]);
		}
		if (count > 0 && static_cast<int>(matched.size()) > count)
			matched.resize(static_cast<std::size_t>(count));
		return matched;
	}

	// @p and @a: sort by distance to origin (reversed if count < 0)
	std::sort(matched.begin(), matched.end(), [&](EntityPlayer *a, EntityPlayer *b) {
		return distSq(a) < distSq(b);
	});
	if (count < 0)
	{
		std::reverse(matched.begin(), matched.end());
		count = -count;
	}
	if (count > 0 && static_cast<int>(matched.size()) > count)
		matched.resize(static_cast<std::size_t>(count));
	return matched;
}

bool PlayerSelector::matchesMultiplePlayers(const std::string &token)
{
	char selType = '\0';
	std::string argBody;
	if (!parseSelectorToken(token, selType, argBody))
		return false;
	const std::map<std::string, std::string> args = parseArgumentMap(argBody);
	int count = (selType == 'a') ? 0 : 1;
	if (auto it = args.find("c"); it != args.end())
		count = parseIntDefault(it->second, count);
	return count != 1;
}

bool PlayerSelector::hasTheseArguments(const std::string &token, char expectedType)
{
	char selType = '\0';
	std::string argBody;
	if (!parseSelectorToken(token, selType, argBody))
		return false;
	return expectedType == '\0' || expectedType == selType;
}

bool PlayerSelector::hasArguments(const std::string &token)
{
	return hasTheseArguments(token, '\0');
}
