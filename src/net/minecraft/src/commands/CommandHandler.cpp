#include "CommandHandler.h"

#include <algorithm>
#include <sstream>

#include "CommandBase.h"
#include "CommandExceptions.h"
#include "EntityPlayer.h"
#include "ICommandSender.h"
#include "PlayerSelector.h"
#include "java/String.h"
#include "platform/Log.h"

namespace
{
const std::string kChatRed = "\xc2\xa7" "c";

std::vector<std::string> splitBySpace(const std::string &text, bool keepTrailingEmpty)
{
	std::vector<std::string> parts;
	std::size_t start = 0;
	while (start <= text.size())
	{
		const std::size_t sp = text.find(' ', start);
		if (sp == std::string::npos)
		{
			parts.push_back(text.substr(start));
			break;
		}
		parts.push_back(text.substr(start, sp - start));
		start = sp + 1;
	}
	if (!keepTrailingEmpty)
	{
		while (!parts.empty() && parts.back().empty())
			parts.pop_back();
	}
	return parts;
}
} // namespace

int CommandHandler::executeCommand(ICommandSender *sender, const std::string &rawCommand)
{
	if (sender == nullptr)
		return 0;

	std::string cmdLine = String::trimJava(rawCommand);
	if (!cmdLine.empty() && cmdLine[0] == '/')
		cmdLine = cmdLine.substr(1);
	if (cmdLine.empty())
		return 0;

	std::vector<std::string> tokens = splitBySpace(cmdLine, false);
	if (tokens.empty())
		return 0;

	const std::string cmdName = tokens[0];
	std::vector<std::string> args(tokens.begin() + 1, tokens.end());

	auto it = commandMap.find(cmdName);
	ICommand *cmd = (it != commandMap.end()) ? it->second : nullptr;
	const int usernameIdx = getUsernameIndex(cmd, args);
	int executedCount = 0;

	try
	{
		if (cmd == nullptr)
			throw CommandNotFoundException();

		if (cmd->canCommandSenderUseCommand(sender))
		{
			if (usernameIdx > -1)
			{
				std::vector<EntityPlayer *> targets = PlayerSelector::matchPlayers(sender, args[static_cast<std::size_t>(usernameIdx)]);
				const std::string originalToken = args[static_cast<std::size_t>(usernameIdx)];
				for (EntityPlayer *target : targets)
				{
					args[static_cast<std::size_t>(usernameIdx)] = target->username;
					try
					{
						cmd->processCommand(sender, args);
						++executedCount;
					}
					catch (const CommandException &ex)
					{
						sender->sendChatToPlayer(kChatRed + sender->translateString(ex.getMessageKey(), ex.getErrorArgs()));
					}
				}
				args[static_cast<std::size_t>(usernameIdx)] = originalToken;
			}
			else
			{
				cmd->processCommand(sender, args);
				++executedCount;
			}
		}
		else
		{
			sender->sendChatToPlayer(kChatRed + "You do not have permission to use this command.");
		}
	}
	catch (const WrongUsageException &ex)
	{
		const std::string inner = sender->translateString(ex.getMessageKey(), ex.getErrorArgs());
		sender->sendChatToPlayer(kChatRed + sender->translateString("commands.generic.usage", {inner}));
	}
	catch (const CommandException &ex)
	{
		sender->sendChatToPlayer(kChatRed + sender->translateString(ex.getMessageKey(), ex.getErrorArgs()));
	}
	catch (const std::exception &ex)
	{
		sender->sendChatToPlayer(kChatRed + sender->translateString("commands.generic.exception"));
		MC_LOG_ERROR("cmd", "Exception executing '%s': %s\n", rawCommand.c_str(), ex.what());
	}
	catch (...)
	{
		sender->sendChatToPlayer(kChatRed + sender->translateString("commands.generic.exception"));
	}

	return executedCount;
}

ICommand *CommandHandler::registerCommand(std::unique_ptr<ICommand> command)
{
	if (!command)
		return nullptr;
	ICommand *raw = command.get();
	const std::string primaryName = raw->getCommandName();
	commandMap[primaryName] = raw;
	for (const std::string &alias : raw->getCommandAliases())
	{
		auto it = commandMap.find(alias);
		if (it == commandMap.end() || it->second->getCommandName() != alias)
			commandMap[alias] = raw;
	}
	ownedCommands.push_back(std::move(command));
	return raw;
}

std::vector<std::string> CommandHandler::getPossibleCommands(ICommandSender *sender, const std::string &partialInput) const
{
	std::vector<std::string> out;
	if (sender == nullptr)
		return out;

	std::string input = partialInput;
	if (!input.empty() && input[0] == '/')
		input = input.substr(1);

	std::vector<std::string> tokens = splitBySpace(input, true);
	if (tokens.empty())
		return out;

	const std::string &cmdName = tokens[0];
	if (tokens.size() == 1)
	{
		for (const auto &kv : commandMap)
		{
			if (CommandBase::doesStringStartWith(cmdName, kv.first) && kv.second->canCommandSenderUseCommand(sender))
				out.push_back(kv.first);
		}
		return out;
	}

	auto it = commandMap.find(cmdName);
	if (it != commandMap.end() && it->second != nullptr && it->second->canCommandSenderUseCommand(sender))
	{
		std::vector<std::string> args(tokens.begin() + 1, tokens.end());
		return it->second->addTabCompletionOptions(sender, args);
	}
	return out;
}

std::vector<ICommand *> CommandHandler::getPossibleCommands(ICommandSender *sender) const
{
	std::vector<ICommand *> out;
	if (sender == nullptr)
		return out;
	for (const auto &cmd : ownedCommands)
	{
		if (cmd && cmd->canCommandSenderUseCommand(sender))
			out.push_back(cmd.get());
	}
	return out;
}

int CommandHandler::getUsernameIndex(ICommand *cmd, const std::vector<std::string> &args) const
{
	if (cmd == nullptr)
		return -1;
	for (int i = 0; i < static_cast<int>(args.size()); ++i)
	{
		if (cmd->isUsernameIndex(args, i) && PlayerSelector::matchesMultiplePlayers(args[static_cast<std::size_t>(i)]))
			return i;
	}
	return -1;
}
