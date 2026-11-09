#pragma once

#include "Render.h"

class EntityWitherSkull;
class ModelSkeletonHead;

// net.minecraft.src.RenderWitherSkull
class RenderWitherSkull : public Render
{
public:
	RenderWitherSkull();
	~RenderWitherSkull() override;

	void doRender(Entity *entity, double x, double y, double z, float yaw, float partialTick) override;

private:
	ModelSkeletonHead *skeletonHeadModel;
};
