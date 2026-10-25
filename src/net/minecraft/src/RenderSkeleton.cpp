#include "RenderSkeleton.h"

#include "EntitySkeleton.h"
#include "ModelSkeleton.h"
#include "platform/RenderAPI.h"

RenderSkeleton::RenderSkeleton()
	: RenderBiped(new ModelSkeleton(), 0.5f)
{
}

void RenderSkeleton::preRenderCallback(EntityLiving *entityLiving, float f)
{
	EntitySkeleton *skeleton = dynamic_cast<EntitySkeleton *>(entityLiving);
	if (skeleton != nullptr && skeleton->getSkeletonType() == 1)
	{
		renderScale(1.2f, 1.2f, 1.2f);
	}
}

void RenderSkeleton::renderEquippedItemTranslate()
{
	renderTranslate(0.09375f, 0.1875f, 0.0f);
}
