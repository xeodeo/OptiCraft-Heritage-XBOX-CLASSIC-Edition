#pragma once

#include <string>

#include "java/Type.h"

// net.minecraft.src.BossStatus: values are copied, never an entity pointer, so a boss that dies or
// unloads between its render and the HUD cannot leave a dangling reference.
class BossStatus
{
public:
	static float healthScale;
	static int_t statusBarLength;
	static std::string bossName;
	static bool field_82825_d;   // darken the sky (Wither)

	static void func_82824_a(int_t bossHealth, int_t maxHealth, const std::string &name, bool darkenSky);
};
