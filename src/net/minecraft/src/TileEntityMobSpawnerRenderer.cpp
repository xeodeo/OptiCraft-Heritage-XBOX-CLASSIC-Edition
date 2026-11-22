#include "TileEntityMobSpawnerRenderer.h"

#include "Entity.h"
#include "MobSpawnerBaseLogic.h"
#include "RenderManager.h"
#include "TileEntityMobSpawner.h"
#include "World.h"

#include "platform/PlatformTuning.h"
#include "platform/RenderAPI.h"

TileEntityMobSpawnerRenderer::TileEntityMobSpawnerRenderer()
{
}

TileEntityMobSpawnerRenderer::~TileEntityMobSpawnerRenderer()
{
}

void TileEntityMobSpawnerRenderer::renderMobSpawnerLogic(MobSpawnerBaseLogic *logic, double d, double d1, double d2, float f)
{
	if (logic == nullptr)
		return;

	Entity *entity = logic->getCachedEntity();
	if (entity != nullptr)
	{
		entity->setWorld(logic->getSpawnerWorld());
		const float scale = 0.4375f;
		renderTranslate(0.0f, 0.4f, 0.0f);
#if PLATFORM_FLOAT_VERTEX_MATH
		const float interpolatedYaw = static_cast<float>(logic->yaw2) +
			(static_cast<float>(logic->yaw) - static_cast<float>(logic->yaw2)) * f;
		renderRotate(interpolatedYaw * 10.0f, 0.0f, 1.0f, 0.0f);
#else
		renderRotate((float)(logic->yaw2 + (logic->yaw - logic->yaw2) * (double)f) * 10.0f, 0.0f, 1.0f, 0.0f);
#endif
		renderRotate(-30.0f, 1.0f, 0.0f, 0.0f);
		renderTranslate(0.0f, -0.4f, 0.0f);
		renderScale(scale, scale, scale);
		entity->setLocationAndAngles(d, d1, d2, 0.0f, 0.0f);
		RenderManager::instance->renderEntityWithPosYaw(entity, 0.0, 0.0, 0.0, 0.0f, f);
	}
}

void TileEntityMobSpawnerRenderer::renderTileEntityMobSpawner(TileEntityMobSpawner *tileentitymobspawner, double d, double d1, double d2, float f)
{
	renderPushMatrix();
	renderTranslate((float)d + 0.5f, (float)d1, (float)d2 + 0.5f);
	renderMobSpawnerLogic(tileentitymobspawner->getSpawnerLogic(), d, d1, d2, f);
	renderPopMatrix();
}

void TileEntityMobSpawnerRenderer::renderTileEntityAt(TileEntity *tileentity, double d, double d1, double d2, float f)
{
	renderTileEntityMobSpawner(static_cast<TileEntityMobSpawner *>(tileentity), d, d1, d2, f);
}
