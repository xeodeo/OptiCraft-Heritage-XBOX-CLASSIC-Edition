#include "RenderWitherSkull.h"

#include "EntityWitherSkull.h"
#include "ModelSkeletonHead.h"
#include "platform/RenderAPI.h"

namespace
{
float interpolateYaw(float previous, float current, float partialTick)
{
	float delta = current - previous;
	while (delta < -180.0f)
		delta += 360.0f;
	while (delta >= 180.0f)
		delta -= 360.0f;
	return previous + partialTick * delta;
}
}

RenderWitherSkull::RenderWitherSkull()
	: skeletonHeadModel(new ModelSkeletonHead())
{
}

RenderWitherSkull::~RenderWitherSkull()
{
	delete skeletonHeadModel;
}

void RenderWitherSkull::doRender(Entity *entity, double x, double y, double z, float yaw, float partialTick)
{
	(void)yaw;
	EntityWitherSkull *skull = static_cast<EntityWitherSkull *>(entity);
	renderPushMatrix();
	renderDisable(RenderCapability::CullFace);
	const float skullYaw = interpolateYaw(skull->prevRotationYaw, skull->rotationYaw, partialTick);
	const float skullPitch = skull->prevRotationPitch + (skull->rotationPitch - skull->prevRotationPitch) * partialTick;
	renderTranslate(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
	renderEnable(RenderCapability::RescaleNormal);
	renderScale(-1.0f, -1.0f, 1.0f);
	renderEnable(RenderCapability::AlphaTest);
	loadTexture(skull->isInvulnerable() ? "/mob/wither_invul.png" : "/mob/wither.png");
	skeletonHeadModel->render(skull, 0.0f, 0.0f, 0.0f, skullYaw, skullPitch, 0.0625f);
	renderDisable(RenderCapability::RescaleNormal);
	renderEnable(RenderCapability::CullFace);
	renderPopMatrix();
}
