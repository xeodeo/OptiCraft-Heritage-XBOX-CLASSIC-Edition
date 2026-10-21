#pragma once

#include "ModelBase.h"

class ModelRenderer;

// net.minecraft.src.ModelSkeletonHead
class ModelSkeletonHead : public ModelBase
{
public:
	ModelSkeletonHead();
	ModelSkeletonHead(int_t offsetX, int_t offsetY, int_t texWidth, int_t texHeight);
	~ModelSkeletonHead() override;

	void render(Entity *entity, float f, float f1, float f2, float f3, float f4, float f5) override;
	void setRotationAngles(float f, float f1, float f2, float f3, float f4, float f5) override;

	ModelRenderer *skeletonHead;
};
