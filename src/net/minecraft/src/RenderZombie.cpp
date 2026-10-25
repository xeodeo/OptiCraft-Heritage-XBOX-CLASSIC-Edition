#include "RenderZombie.h"

#include <cmath>
#include "EntityZombie.h"
#include "ModelZombie.h"
#include "ModelZombieVillager.h"

RenderZombie::RenderZombie()
	: RenderBiped(new ModelZombie(), 0.5f)
{
	zombieModel = static_cast<ModelZombie *>(mainModel);
	zombieVillagerModel = new ModelZombieVillager();
}

RenderZombie::~RenderZombie()
{
	mainModel = nullptr;
	delete zombieModel;
	delete zombieVillagerModel;
}

void RenderZombie::setupZombieModel(EntityZombie *zombie)
{
	if (zombie != nullptr && zombie->isVillager())
	{
		mainModel = zombieVillagerModel;
	}
	else
	{
		mainModel = zombieModel;
	}
	modelBipedMain = static_cast<ModelBiped *>(mainModel);
}

void RenderZombie::doRenderLiving(EntityLiving *entityLiving, double d, double d1, double d2, float f, float f1)
{
	setupZombieModel(dynamic_cast<EntityZombie *>(entityLiving));
	RenderBiped::doRenderLiving(entityLiving, d, d1, d2, f, f1);
}

void RenderZombie::renderEquippedItems(EntityLiving *entityLiving, float f)
{
	setupZombieModel(dynamic_cast<EntityZombie *>(entityLiving));
	RenderBiped::renderEquippedItems(entityLiving, f);
}

void RenderZombie::rotateCorpse(EntityLiving *entityLiving, float f, float f1, float f2)
{
	EntityZombie *zombie = dynamic_cast<EntityZombie *>(entityLiving);
	if (zombie != nullptr && zombie->isConverting())
	{
		f1 += static_cast<float>(std::cos(static_cast<double>(zombie->ticksExisted) * 3.25) * 3.141592653589793 * 0.25);
	}
	RenderBiped::rotateCorpse(entityLiving, f, f1, f2);
}
