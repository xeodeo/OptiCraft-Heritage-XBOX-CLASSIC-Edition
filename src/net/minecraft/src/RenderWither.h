#pragma once

#include "RenderLiving.h"

class EntityWither;

// net.minecraft.src.RenderWither
class RenderWither : public RenderLiving
{
public:
	RenderWither();

	void doRenderLiving(EntityLiving *entity, double x, double y, double z, float yaw, float partialTick) override;

protected:
	void preRenderCallback(EntityLiving *entity, float partialTick) override;
	int shouldRenderPass(EntityLiving *entity, int pass, float partialTick) override;
	int inheritRenderPass(EntityLiving *entity, int pass, float partialTick) override;
};
