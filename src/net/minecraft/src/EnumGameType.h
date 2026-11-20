#pragma once

#include <string>
#include "java/Type.h"

class PlayerCapabilities;

// net.minecraft.src.EnumGameType (Java 1.5.2)
class EnumGameType
{
public:
	static constexpr int_t NOT_SET = -1;
	static constexpr int_t SURVIVAL = 0;
	static constexpr int_t CREATIVE = 1;
	static constexpr int_t ADVENTURE = 2;

	static int_t getByID(int_t id);
	static int_t getByName(const std::string &name);
	static std::string getName(int_t gameType);
	static void configurePlayerCapabilities(int_t gameType, PlayerCapabilities &capabilities);
	static bool isAdventure(int_t gameType) { return gameType == ADVENTURE; }
	static bool isCreative(int_t gameType) { return gameType == CREATIVE; }
	static bool isSurvivalOrAdventure(int_t gameType) { return gameType == SURVIVAL || gameType == ADVENTURE; }
};
