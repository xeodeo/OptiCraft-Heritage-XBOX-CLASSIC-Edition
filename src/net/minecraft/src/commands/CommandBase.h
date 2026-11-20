#pragma once

#include <climits>
#include <string>
#include <vector>
#include "ICommand.h"
#include "ICommandSender.h"
#include "CommandExceptions.h"

class EntityPlayer;
class Minecraft;

class IAdminCommand
{
public:
	virtual ~IAdminCommand() = default;
	virtual void notifyAdmins(ICommandSender *sender, int flags, const std::string &translationKey, const std::vector<std::string> &args) = 0;
};

class CommandBase : public ICommand
{
public:
	virtual int getRequiredPermissionLevel() const { return 4; }
	std::string getCommandUsage(ICommandSender *sender) const override;
	bool canCommandSenderUseCommand(ICommandSender *sender) const override;

	static int parseInt(ICommandSender *sender, const std::string &str);
	static int parseIntWithMin(ICommandSender *sender, const std::string &str, int minVal);
	static int parseIntBounded(ICommandSender *sender, const std::string &str, int minVal, int maxVal);
	static double parseDouble(ICommandSender *sender, const std::string &str);
	static std::string formatDouble2(double v);

	static EntityPlayer *getCommandSenderAsPlayer(ICommandSender *sender);
	static EntityPlayer *getPlayer(ICommandSender *sender, const std::string &token);
	static std::string getPlayerName(ICommandSender *sender, const std::string &token);
	static std::string joinArgs(ICommandSender *sender, const std::vector<std::string> &args, int startIndex, bool allowSelectors = false);
	static std::string joinNiceString(const std::vector<std::string> &items);
	static bool doesStringStartWith(const std::string &prefix, const std::string &candidate);
	static std::vector<std::string> getListOfStringsMatchingLastWord(const std::vector<std::string> &args, const std::vector<std::string> &candidates);

	static std::vector<EntityPlayer *> getAllPlayers(Minecraft *mc);
	static std::vector<std::string> getAllUsernames(Minecraft *mc);

	static void notifyAdmins(ICommandSender *sender, const std::string &translationKey, const std::vector<std::string> &args = {});
	static void notifyAdmins(ICommandSender *sender, int flags, const std::string &translationKey, const std::vector<std::string> &args = {});
	static void setAdminCommander(IAdminCommand *admin);

private:
	static IAdminCommand *theAdmin;
};
