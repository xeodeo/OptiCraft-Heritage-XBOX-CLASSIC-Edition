#include "CommandBase.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "ChunkCoordinates.h"
#include "EntityPlayer.h"
#include "EntityPlayerSP.h"
#include "GuiIngame.h"
#include "MathHelper.h"
#include "Minecraft.h"
#include "PlayerSelector.h"
#include "StringTranslate.h"
#include "World.h"
#include "WorldInfo.h"
#include "java/String.h"

IAdminCommand *CommandBase::theAdmin = nullptr;

PlayerCommandSender::PlayerCommandSender(Minecraft *minecraft, EntityPlayer *senderPlayer)
	: mc(minecraft), player(senderPlayer)
{
}

std::string PlayerCommandSender::getCommandSenderName() const
{
	if (player != nullptr && !player->username.empty())
		return player->username;
	return "Player";
}

void PlayerCommandSender::sendChatToPlayer(const std::string &message)
{
	if (mc != nullptr && mc->ingameGUI != nullptr)
		mc->ingameGUI->addChatMessage(message);
}

bool PlayerCommandSender::canCommandSenderUseCommand(int permissionLevel, const std::string &commandName) const
{
	(void)commandName;
	if (permissionLevel <= 0)
		return true;
	if (mc != nullptr && mc->theWorld != nullptr && mc->theWorld->getWorldInfo() != nullptr)
		return mc->theWorld->getWorldInfo()->areCommandsAllowed();
	return false;
}

std::string PlayerCommandSender::translateString(const std::string &key, const std::vector<std::string> &args) const
{
	StringTranslate *tr = StringTranslate::getInstance();
	if (tr == nullptr)
		return key;
	if (args.empty())
		return tr->translateKey(key);
	return tr->translateKeyFormat(key, args);
}

ChunkCoordinates PlayerCommandSender::getPlayerCoordinates() const
{
	if (player != nullptr)
	{
		return ChunkCoordinates(
			MathHelper::floor_double(player->posX),
			MathHelper::floor_double(player->boundingBox != nullptr ? player->boundingBox->minY : player->posY),
			MathHelper::floor_double(player->posZ));
	}
	return ChunkCoordinates(0, 64, 0);
}

std::string CommandBase::getCommandUsage(ICommandSender *sender) const
{
	(void)sender;
	return "/" + getCommandName();
}

bool CommandBase::canCommandSenderUseCommand(ICommandSender *sender) const
{
	return sender != nullptr && sender->canCommandSenderUseCommand(getRequiredPermissionLevel(), getCommandName());
}

int CommandBase::parseInt(ICommandSender *sender, const std::string &str)
{
	(void)sender;
	if (str.empty())
		throw NumberInvalidException("commands.generic.num.invalid", {str});
	char *endPtr = nullptr;
	const long long val = std::strtoll(str.c_str(), &endPtr, 10);
	if (endPtr == nullptr || *endPtr != '\0' || val < INT_MIN || val > INT_MAX)
		throw NumberInvalidException("commands.generic.num.invalid", {str});
	return static_cast<int>(val);
}

int CommandBase::parseIntWithMin(ICommandSender *sender, const std::string &str, int minVal)
{
	return parseIntBounded(sender, str, minVal, INT_MAX);
}

int CommandBase::parseIntBounded(ICommandSender *sender, const std::string &str, int minVal, int maxVal)
{
	const int val = parseInt(sender, str);
	if (val < minVal)
		throw NumberInvalidException("commands.generic.num.tooSmall", {std::to_string(val), std::to_string(minVal)});
	if (val > maxVal)
		throw NumberInvalidException("commands.generic.num.tooBig", {std::to_string(val), std::to_string(maxVal)});
	return val;
}

double CommandBase::parseDouble(ICommandSender *sender, const std::string &str)
{
	(void)sender;
	if (str.empty())
		throw NumberInvalidException("commands.generic.double.invalid", {str});
	char *endPtr = nullptr;
	const double val = std::strtod(str.c_str(), &endPtr);
	if (endPtr == nullptr || *endPtr != '\0' || !std::isfinite(val))
		throw NumberInvalidException("commands.generic.double.invalid", {str});
	return val;
}

std::string CommandBase::formatDouble2(double v)
{
	char buf[48];
	std::snprintf(buf, sizeof(buf), "%.2f", v);
	return std::string(buf);
}

EntityPlayer *CommandBase::getCommandSenderAsPlayer(ICommandSender *sender)
{
	EntityPlayer *p = sender != nullptr ? sender->getSenderPlayer() : nullptr;
	if (p != nullptr)
		return p;
	throw PlayerNotFoundException("You must specify which player you wish to perform this action on.");
}

std::vector<EntityPlayer *> CommandBase::getAllPlayers(Minecraft *mc)
{
	std::vector<EntityPlayer *> out;
	if (mc == nullptr)
		return out;
	EntityPlayer *p1 = mc->thePlayerOne != nullptr ? static_cast<EntityPlayer *>(mc->thePlayerOne)
	                                               : static_cast<EntityPlayer *>(mc->thePlayer);
	if (p1 != nullptr)
		out.push_back(p1);
	if (mc->thePlayer2 != nullptr && mc->thePlayer2 != p1)
		out.push_back(mc->thePlayer2);
	if (mc->theWorld != nullptr)
	{
		for (EntityPlayer *wp : mc->theWorld->playerEntities)
		{
			if (wp != nullptr && std::find(out.begin(), out.end(), wp) == out.end())
				out.push_back(wp);
		}
	}
	return out;
}

std::vector<std::string> CommandBase::getAllUsernames(Minecraft *mc)
{
	std::vector<std::string> names;
	for (EntityPlayer *p : getAllPlayers(mc))
	{
		if (p != nullptr && !p->username.empty())
			names.push_back(p->username);
	}
	return names;
}

EntityPlayer *CommandBase::getPlayer(ICommandSender *sender, const std::string &token)
{
	EntityPlayer *selected = PlayerSelector::matchOnePlayer(sender, token);
	if (selected != nullptr)
		return selected;

	Minecraft *mc = sender != nullptr ? sender->getMinecraft() : nullptr;
	const std::string lowerToken = String::toLowerCaseJava(token);
	for (EntityPlayer *p : getAllPlayers(mc))
	{
		if (p != nullptr && static_cast<std::string>(String::toLowerCaseJava(p->username)) == lowerToken)
			return p;
	}
	throw PlayerNotFoundException();
}

std::string CommandBase::getPlayerName(ICommandSender *sender, const std::string &token)
{
	EntityPlayer *selected = PlayerSelector::matchOnePlayer(sender, token);
	if (selected != nullptr)
		return selected->username;
	if (PlayerSelector::hasArguments(token))
		throw PlayerNotFoundException();
	return token;
}

std::string CommandBase::joinArgs(ICommandSender *sender, const std::vector<std::string> &args, int startIndex, bool allowSelectors)
{
	std::string out;
	for (int i = startIndex; i < static_cast<int>(args.size()); ++i)
	{
		if (i > startIndex)
			out += ' ';
		std::string part = args[static_cast<std::size_t>(i)];
		if (allowSelectors)
		{
			std::string resolved = PlayerSelector::matchPlayersAsString(sender, part);
			if (!resolved.empty())
				part = resolved;
			else if (PlayerSelector::hasArguments(part))
				throw PlayerNotFoundException();
		}
		out += part;
	}
	return out;
}

std::string CommandBase::joinNiceString(const std::vector<std::string> &items)
{
	std::string out;
	for (std::size_t i = 0; i < items.size(); ++i)
	{
		if (i > 0)
		{
			if (i + 1 == items.size())
				out += " and ";
			else
				out += ", ";
		}
		out += items[i];
	}
	return out;
}

bool CommandBase::doesStringStartWith(const std::string &prefix, const std::string &candidate)
{
	if (prefix.size() > candidate.size())
		return false;
	for (std::size_t i = 0; i < prefix.size(); ++i)
	{
		if (std::tolower(static_cast<unsigned char>(prefix[i])) !=
		    std::tolower(static_cast<unsigned char>(candidate[i])))
			return false;
	}
	return true;
}

std::vector<std::string> CommandBase::getListOfStringsMatchingLastWord(
	const std::vector<std::string> &args, const std::vector<std::string> &candidates)
{
	std::vector<std::string> out;
	const std::string last = args.empty() ? std::string() : args.back();
	for (const std::string &cand : candidates)
	{
		if (doesStringStartWith(last, cand))
			out.push_back(cand);
	}
	return out;
}

void CommandBase::notifyAdmins(ICommandSender *sender, const std::string &translationKey, const std::vector<std::string> &args)
{
	notifyAdmins(sender, 0, translationKey, args);
}

void CommandBase::notifyAdmins(ICommandSender *sender, int flags, const std::string &translationKey, const std::vector<std::string> &args)
{
	if (theAdmin != nullptr)
		theAdmin->notifyAdmins(sender, flags, translationKey, args);
}

void CommandBase::setAdminCommander(IAdminCommand *admin)
{
	theAdmin = admin;
}
