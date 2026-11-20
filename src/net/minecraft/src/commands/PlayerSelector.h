#pragma once

#include <string>
#include <vector>

class EntityPlayer;
class ICommandSender;

// net.minecraft.src.PlayerSelector (Java 1.5.2)
class PlayerSelector
{
public:
	static EntityPlayer *matchOnePlayer(ICommandSender *sender, const std::string &token);
	static std::string matchPlayersAsString(ICommandSender *sender, const std::string &token);
	static std::vector<EntityPlayer *> matchPlayers(ICommandSender *sender, const std::string &token, bool *matchedSyntax = nullptr);
	static bool matchesMultiplePlayers(const std::string &token);
	static bool hasTheseArguments(const std::string &token, char expectedType);
	static bool hasArguments(const std::string &token);
};
