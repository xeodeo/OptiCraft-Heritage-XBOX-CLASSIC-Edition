#include "RenderWolf.h"
#include "EntitySheep.h"
#include "EntityWolf.h"
#include "ModelBase.h"
#include "platform/RenderAPI.h"

RenderWolf::RenderWolf(ModelBase *modelBase, ModelBase *renderPassModel, float shadowSize)
	: RenderLiving(modelBase, shadowSize)
{
	setRenderPassModel(renderPassModel);
}

RenderWolf::~RenderWolf()
{
	delete renderPassModel;
	renderPassModel = nullptr;
}

void RenderWolf::renderWolf(EntityWolf *wolf, double x, double y, double z, float yaw, float partialTicks)
{
	RenderLiving::doRenderLiving(wolf, x, y, z, yaw, partialTicks);
}

void RenderWolf::doRenderLiving(EntityLiving *entity, double x, double y, double z, float yaw, float partialTicks)
{
	renderWolf(static_cast<EntityWolf *>(entity), x, y, z, yaw, partialTicks);
}

void RenderWolf::doRender(Entity *entity, double x, double y, double z, float yaw, float partialTicks)
{
	renderWolf(static_cast<EntityWolf *>(entity), x, y, z, yaw, partialTicks);
}

float RenderWolf::getTicksExisted(EntityLiving *entity, float)
{
	return static_cast<EntityWolf *>(entity)->setTailRotation();
}

int RenderWolf::func_82447_a(EntityWolf *wolf, int pass, float partialTicks)
{
	if (pass == 0 && wolf->getWolfShaking())
	{
		float shading = wolf->getEntityBrightness(partialTicks) * wolf->getShadingWhileShaking(partialTicks);
		loadTexture(wolf->getEntityTexture());
		renderColor3f(shading, shading, shading);
		return 1;
	}
	if (pass == 1 && wolf->isTamed())
	{
		loadTexture("/mob/wolf_collar.png");
		float brightness = 1.0f;
		int color = wolf->getCollarColor();
		renderColor3f(
			brightness * EntitySheep::fleeceColorTable[color][0],
			brightness * EntitySheep::fleeceColorTable[color][1],
			brightness * EntitySheep::fleeceColorTable[color][2]);
		return 1;
	}
	return -1;
}

int RenderWolf::shouldRenderPass(EntityLiving *entity, int pass, float partialTicks)
{
	return func_82447_a(static_cast<EntityWolf *>(entity), pass, partialTicks);
}

void RenderWolf::preRenderCallback(EntityLiving *, float) {}
