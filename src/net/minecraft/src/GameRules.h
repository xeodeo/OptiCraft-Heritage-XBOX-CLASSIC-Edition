#pragma once

#include <map>
#include <string>
#include <vector>

class NBTTagCompound;

// net.minecraft.src.GameRuleValue (Java 1.5.2)
class GameRuleValue
{
public:
	GameRuleValue() = default;
	explicit GameRuleValue(const std::string &value);

	void setValue(const std::string &value);
	const std::string &getGameRuleStringValue() const { return valueString; }
	bool getGameRuleBooleanValue() const { return valueBoolean; }
	int getGameRuleIntValue() const { return valueInteger; }
	double getGameRuleDoubleValue() const { return valueDouble; }

private:
	std::string valueString;
	bool valueBoolean = false;
	int valueInteger = 0;
	double valueDouble = 0.0;
};

// net.minecraft.src.GameRules (Java 1.5.2)
class GameRules
{
public:
	GameRules();

	void addGameRule(const std::string &name, const std::string &value);
	void setOrCreateGameRule(const std::string &name, const std::string &value);
	std::string getGameRuleStringValue(const std::string &name) const;
	bool getGameRuleBooleanValue(const std::string &name) const;
	NBTTagCompound *writeGameRulesToNBT() const;
	void readGameRulesFromNBT(NBTTagCompound *tag);
	std::vector<std::string> getRules() const;
	bool hasRule(const std::string &name) const;

private:
	std::map<std::string, GameRuleValue> theGameRules;
};
