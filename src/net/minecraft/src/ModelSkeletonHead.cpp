#include "ModelSkeletonHead.h"

#include "ModelRenderer.h"

ModelSkeletonHead::ModelSkeletonHead()
	: ModelSkeletonHead(0, 35, 64, 64)
{
}

ModelSkeletonHead::ModelSkeletonHead(int_t offsetX, int_t offsetY, int_t texWidth, int_t texHeight)
{
	textureWidth = texWidth;
	textureHeight = texHeight;
	skeletonHead = new ModelRenderer(this, offsetX, offsetY);
	skeletonHead->addBox(-4.0f, -8.0f, -4.0f, 8, 8, 8, 0.0f);
	skeletonHead->setRotationPoint(0.0f, 0.0f, 0.0f);
}

ModelSkeletonHead::~ModelSkeletonHead()
{
	delete skeletonHead;
}

void ModelSkeletonHead::render(Entity *entity, float f, float f1, float f2, float f3, float f4, float f5)
{
	(void)entity;
	setRotationAngles(f, f1, f2, f3, f4, f5);
	skeletonHead->render(f5);
}

void ModelSkeletonHead::setRotationAngles(float f, float f1, float f2, float f3, float f4, float f5)
{
	ModelBase::setRotationAngles(f, f1, f2, f3, f4, f5);
	skeletonHead->rotateAngleY = f3 / (180.0f / 3.14159265358979323846f);
	skeletonHead->rotateAngleX = f4 / (180.0f / 3.14159265358979323846f);
}
