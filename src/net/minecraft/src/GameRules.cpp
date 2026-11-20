#include "GameRules.h"

#include <cstdlib>
#include "NBTBase.h"
#include "NBTTagCompound.h"
#include "java/String.h"

GameRuleValue::GameRuleValue(const std::string &value)
{
	setValue(value);
}

void GameRuleValue::setValue(const std::string &value)
{
	valueString = value;
	valueBoolean = (String::toLowerCaseJava(value) == "true");

	if (!value.empty())
	{
		char *endPtr = nullptr;
		const long parsedInt = std::strtol(value.c_str(), &endPtr, 10);
		if (endPtr != nullptr && *endPtr == '\0')
			valueInteger = static_cast<int>(parsedInt);

		endPtr = nullptr;
		const double parsedDouble = std::strtod(value.c_str(), &endPtr);
		if (endPtr != nullptr && *endPtr == '\0')
			valueDouble = parsedDouble;
	}
}

GameRules::GameRules()
{
	addGameRule("doFireTick", "true");
	addGameRule("mobGriefing", "true");
	addGameRule("keepInventory", "false");
	addGameRule("doMobSpawning", "true");
	addGameRule("doMobLoot", "true");
	addGameRule("doTileDrops", "true");
	addGameRule("commandBlockOutput", "true");
}

void GameRules::addGameRule(const std::string &name, const std::string &value)
{
	theGameRules[name] = GameRuleValue(value);
}

void GameRules::setOrCreateGameRule(const std::string &name, const std::string &value)
{
	auto it = theGameRules.find(name);
	if (it != theGameRules.end())
		it->second.setValue(value);
	else
		addGameRule(name, value);
}

std::string GameRules::getGameRuleStringValue(const std::string &name) const
{
	auto it = theGameRules.find(name);
	return it != theGameRules.end() ? it->second.getGameRuleStringValue() : "";
}

bool GameRules::getGameRuleBooleanValue(const std::string &name) const
{
	auto it = theGameRules.find(name);
	return it != theGameRules.end() ? it->second.getGameRuleBooleanValue() : false;
}

NBTTagCompound *GameRules::writeGameRulesToNBT() const
{
	NBTTagCompound *tag = new NBTTagCompound();
	tag->setKey("GameRules");
	for (const auto &kv : theGameRules)
		tag->setString(kv.first, kv.second.getGameRuleStringValue());
	return tag;
}

void GameRules::readGameRulesFromNBT(NBTTagCompound *tag)
{
	if (tag == nullptr)
		return;
	for (NBTBase *entry : tag->getTags())
	{
		if (entry == nullptr)
			continue;
		const std::string key = entry->getKey();
		if (!key.empty())
			setOrCreateGameRule(key, tag->getString(key));
	}
}

std::vector<std::string> GameRules::getRules() const
{
	std::vector<std::string> out;
	out.reserve(theGameRules.size());
	for (const auto &kv : theGameRules)
		out.push_back(kv.first);
	return out;
}

bool GameRules::hasRule(const std::string &name) const
{
	return theGameRules.find(name) != theGameRules.end();
}
