#pragma once

#include "RenderLiving.h"

class ModelWitch;

// net.minecraft.src.RenderWitch
class RenderWitch : public RenderLiving
{
public:
	RenderWitch();

	void doRenderLiving(EntityLiving *entityLiving, double d, double d1, double d2, float f, float f1) override;

protected:
	void preRenderCallback(EntityLiving *entityLiving, float f) override;
	void renderEquippedItems(EntityLiving *entityLiving, float f) override;

private:
	ModelWitch *modelWitch;
};
