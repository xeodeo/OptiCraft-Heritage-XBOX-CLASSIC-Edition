#pragma once

#include "RenderLiving.h"

// net.minecraft.src.RenderBat
class RenderBat : public RenderLiving
{
public:
	RenderBat();

protected:
	void preRenderCallback(EntityLiving *entityLiving, float f) override;
	void rotateCorpse(EntityLiving *entityLiving, float f, float f1, float f2) override;
};
