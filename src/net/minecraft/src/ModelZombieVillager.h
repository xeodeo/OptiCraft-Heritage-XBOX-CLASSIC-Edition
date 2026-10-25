#pragma once

#include "ModelBiped.h"

// net.minecraft.src.ModelZombieVillager
class ModelZombieVillager : public ModelBiped
{
public:
	ModelZombieVillager();
	ModelZombieVillager(float f, float f1, bool isArmor);

	int_t func_82897_a() const { return 10; }

	void setRotationAngles(float f, float f1, float f2, float f3, float f4, float f5) override;
};
