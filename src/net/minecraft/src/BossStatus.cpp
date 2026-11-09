#include "BossStatus.h"

float BossStatus::healthScale = 0.0f;
int_t BossStatus::statusBarLength = 0;
std::string BossStatus::bossName;
bool BossStatus::field_82825_d = false;

void BossStatus::func_82824_a(int_t bossHealth, int_t maxHealth, const std::string &name, bool darkenSky)
{
	healthScale = maxHealth > 0 ? static_cast<float>(bossHealth) / static_cast<float>(maxHealth) : 0.0f;
	statusBarLength = 100;
	bossName = name;
	field_82825_d = darkenSky;
}
