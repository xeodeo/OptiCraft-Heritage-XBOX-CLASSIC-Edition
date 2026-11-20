#include "EnumGameType.h"

#include "PlayerCapabilities.h"
#include "java/String.h"

int_t EnumGameType::getByID(int_t id)
{
	if (id == NOT_SET || id == SURVIVAL || id == CREATIVE || id == ADVENTURE)
		return id;
	return SURVIVAL;
}

int_t EnumGameType::getByName(const std::string &name)
{
	const std::string lower = String::toLowerCaseJava(name);
	if (lower == "survival")
		return SURVIVAL;
	if (lower == "creative")
		return CREATIVE;
	if (lower == "adventure")
		return ADVENTURE;
	return SURVIVAL;
}

std::string EnumGameType::getName(int_t gameType)
{
	switch (gameType)
	{
	case CREATIVE:
		return "creative";
	case ADVENTURE:
		return "adventure";
	case SURVIVAL:
		return "survival";
	default:
		return "";
	}
}

void EnumGameType::configurePlayerCapabilities(int_t gameType, PlayerCapabilities &capabilities)
{
	if (gameType == CREATIVE)
	{
		capabilities.allowFlying = true;
		capabilities.isCreativeMode = true;
		capabilities.disableDamage = true;
	}
	else
	{
		capabilities.allowFlying = false;
		capabilities.isCreativeMode = false;
		capabilities.disableDamage = false;
		capabilities.isFlying = false;
	}
	capabilities.allowEdit = !isAdventure(gameType);
}
