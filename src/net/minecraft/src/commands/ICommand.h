#pragma once

#include <string>
#include <vector>

class ICommandSender;

class ICommand
{
public:
	virtual ~ICommand() = default;

	virtual std::string getCommandName() const = 0;
	virtual std::string getCommandUsage(ICommandSender *sender) const = 0;
	virtual std::vector<std::string> getCommandAliases() const { return {}; }
	virtual void processCommand(ICommandSender *sender, const std::vector<std::string> &args) = 0;
	virtual bool canCommandSenderUseCommand(ICommandSender *sender) const = 0;
	virtual std::vector<std::string> addTabCompletionOptions(ICommandSender *sender, const std::vector<std::string> &args) const { (void)sender; (void)args; return {}; }
	virtual bool isUsernameIndex(const std::vector<std::string> &args, int index) const { (void)args; (void)index; return false; }
};
