#pragma once

#include <exception>
#include <string>
#include <vector>

class CommandException : public std::exception
{
public:
	explicit CommandException(const std::string &messageKey, std::vector<std::string> args = {})
		: key(messageKey), errorArgs(std::move(args))
	{
	}

	const char *what() const noexcept override { return key.c_str(); }
	const std::string &getMessageKey() const { return key; }
	const std::vector<std::string> &getErrorArgs() const { return errorArgs; }

private:
	std::string key;
	std::vector<std::string> errorArgs;
};

class WrongUsageException : public CommandException
{
public:
	explicit WrongUsageException(const std::string &messageKey, std::vector<std::string> args = {})
		: CommandException(messageKey, std::move(args))
	{
	}
};

class NumberInvalidException : public CommandException
{
public:
	NumberInvalidException()
		: CommandException("commands.generic.num.invalid")
	{
	}

	explicit NumberInvalidException(const std::string &messageKey, std::vector<std::string> args = {})
		: CommandException(messageKey, std::move(args))
	{
	}
};

class PlayerNotFoundException : public CommandException
{
public:
	PlayerNotFoundException()
		: CommandException("commands.generic.player.notFound")
	{
	}

	explicit PlayerNotFoundException(const std::string &messageKey, std::vector<std::string> args = {})
		: CommandException(messageKey, std::move(args))
	{
	}
};

class CommandNotFoundException : public CommandException
{
public:
	CommandNotFoundException()
		: CommandException("commands.generic.notFound")
	{
	}

	explicit CommandNotFoundException(const std::string &messageKey, std::vector<std::string> args = {})
		: CommandException(messageKey, std::move(args))
	{
	}
};
