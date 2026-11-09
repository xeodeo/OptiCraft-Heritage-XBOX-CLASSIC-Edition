#include "RenderWither.h"

#include "BossStatus.h"
#include "EntityWither.h"
#include "MathHelper.h"
#include "ModelWither.h"
#include "StatCollector.h"
#include "platform/RenderAPI.h"

RenderWither::RenderWither()
	: RenderLiving(new ModelWither(), 1.0f)
{
}

void RenderWither::doRenderLiving(EntityLiving *entity, double x, double y, double z, float yaw, float partialTick)
{
	EntityWither *wither = static_cast<EntityWither *>(entity);
	BossStatus::func_82824_a(wither->getBossHealth(), wither->getMaxHealth(),
		StatCollector::translateToLocal("entity.WitherBoss.name"), true);
	RenderLiving::doRenderLiving(entity, x, y, z, yaw, partialTick);
}

void RenderWither::preRenderCallback(EntityLiving *entity, float partialTick)
{
	EntityWither *wither = static_cast<EntityWither *>(entity);
	const int_t invul = wither->func_82212_n();
	if (invul > 0)
	{
		const float scale = 2.0f - (static_cast<float>(invul) - partialTick) / 220.0f * 0.5f;
		renderScale(scale, scale, scale);
	}
	else
	{
		renderScale(2.0f, 2.0f, 2.0f);
	}
}

// Pass 1 draws the scrolling armour shell (below half health); pass 2 undoes its state.
int RenderWither::shouldRenderPass(EntityLiving *entity, int pass, float partialTick)
{
	EntityWither *wither = static_cast<EntityWither *>(entity);
	if (!wither->isArmored())
		return -1;

	renderDepthMask(!wither->isInvisible());

	if (pass == 1)
	{
		const float time = static_cast<float>(wither->ticksExisted) + partialTick;
		loadTexture("/armor/witherarmor.png");
		renderMatrixMode(RenderMatrixMode::Texture);
		renderLoadIdentity();
		const float u = MathHelper::cos(time * 0.02f) * 3.0f;
		const float v = time * 0.01f;
		renderTranslate(u, v, 0.0f);
		setRenderPassModel(mainModel);
		renderMatrixMode(RenderMatrixMode::ModelView);
		renderEnable(RenderCapability::Blend);
		const float shade = 0.5f;
		renderColor4f(shade, shade, shade, 1.0f);
		renderDisable(RenderCapability::Lighting);
		renderBlendFunc(RenderBlendFactor::One, RenderBlendFactor::One);
		renderTranslate(0.0f, -0.01f, 0.0f);
		renderScale(1.1f, 1.1f, 1.1f);
		return 1;
	}

	if (pass == 2)
	{
		renderMatrixMode(RenderMatrixMode::Texture);
		renderLoadIdentity();
		renderMatrixMode(RenderMatrixMode::ModelView);
		renderEnable(RenderCapability::Lighting);
		renderDisable(RenderCapability::Blend);
	}
	return -1;
}

int RenderWither::inheritRenderPass(EntityLiving *entity, int pass, float partialTick)
{
	(void)entity;
	(void)pass;
	(void)partialTick;
	return -1;
}
