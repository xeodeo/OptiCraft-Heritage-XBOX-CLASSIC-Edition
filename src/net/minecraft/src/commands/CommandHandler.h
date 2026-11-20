#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>
#include "ICommand.h"

class ICommandSender;

// net.minecraft.src.CommandHandler (Java 1.5.2)
class CommandHandler
{
public:
	CommandHandler() = default;
	virtual ~CommandHandler() = default;

	int executeCommand(ICommandSender *sender, const std::string &rawCommand);
	ICommand *registerCommand(std::unique_ptr<ICommand> command);
	std::vector<std::string> getPossibleCommands(ICommandSender *sender, const std::string &partialInput) const;
	std::vector<ICommand *> getPossibleCommands(ICommandSender *sender) const;
	const std::map<std::string, ICommand *> &getCommands() const { return commandMap; }

private:
	int getUsernameIndex(ICommand *cmd, const std::vector<std::string> &args) const;

	std::vector<std::unique_ptr<ICommand>> ownedCommands;
	std::map<std::string, ICommand *> commandMap;
};
