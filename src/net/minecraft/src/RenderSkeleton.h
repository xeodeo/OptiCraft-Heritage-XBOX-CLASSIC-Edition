#pragma once

#include "RenderBiped.h"

// net.minecraft.src.RenderSkeleton
class RenderSkeleton : public RenderBiped
{
public:
	RenderSkeleton();

protected:
	void preRenderCallback(EntityLiving *entityLiving, float f) override;
	void renderEquippedItemTranslate() override;
};
