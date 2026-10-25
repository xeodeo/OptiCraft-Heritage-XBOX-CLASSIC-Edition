#pragma once

#include "RenderBiped.h"

class ModelZombie;
class ModelZombieVillager;
class EntityZombie;

// net.minecraft.src.RenderZombie
class RenderZombie : public RenderBiped
{
public:
	RenderZombie();
	~RenderZombie() override;

	void doRenderLiving(EntityLiving *entityLiving, double d, double d1, double d2, float f, float f1) override;

protected:
	void rotateCorpse(EntityLiving *entityLiving, float f, float f1, float f2) override;
	void renderEquippedItems(EntityLiving *entityLiving, float f) override;

private:
	void setupZombieModel(EntityZombie *zombie);

	ModelZombie *zombieModel;
	ModelZombieVillager *zombieVillagerModel;
};
