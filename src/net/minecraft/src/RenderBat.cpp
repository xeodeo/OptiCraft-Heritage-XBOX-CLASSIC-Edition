#include "RenderBat.h"

#include "EntityBat.h"
#include "MathHelper.h"
#include "ModelBat.h"
#include "platform/RenderAPI.h"

RenderBat::RenderBat()
	: RenderLiving(new ModelBat(), 0.25f)
{
}

void RenderBat::preRenderCallback(EntityLiving *, float)
{
	renderScale(0.35f, 0.35f, 0.35f);
}

void RenderBat::rotateCorpse(EntityLiving *entityLiving, float f, float f1, float f2)
{
	EntityBat *bat = dynamic_cast<EntityBat *>(entityLiving);
	if (bat != nullptr)
	{
		if (!bat->getIsBatHanging())
		{
			renderTranslate(0.0f, MathHelper::cos(f * 0.3f) * 0.1f, 0.0f);
		}
		else
		{
			renderTranslate(0.0f, -0.1f, 0.0f);
		}
	}
	RenderLiving::rotateCorpse(entityLiving, f, f1, f2);
}
