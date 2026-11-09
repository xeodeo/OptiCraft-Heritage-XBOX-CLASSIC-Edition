#include "ModelWither.h"

#include "EntityWither.h"
#include "MathHelper.h"
#include "ModelRenderer.h"

namespace
{
constexpr float kPi = 3.1415927f;
constexpr float kDegToRad = kPi / 180.0f;
}

ModelWither::ModelWither()
{
	textureWidth = 64;
	textureHeight = 64;

	field_82905_a[0] = new ModelRenderer(this, 0, 16);
	field_82905_a[0]->addBox(-10.0f, 3.9f, -0.5f, 20, 3, 3);
	field_82905_a[1] = (new ModelRenderer(this))->setTextureSize(textureWidth, textureHeight);
	field_82905_a[1]->setRotationPoint(-2.0f, 6.9f, -0.5f);
	field_82905_a[1]->setTextureOffset(0, 22)->addBox(0.0f, 0.0f, 0.0f, 3, 10, 3);
	field_82905_a[1]->setTextureOffset(24, 22)->addBox(-4.0f, 1.5f, 0.5f, 11, 2, 2);
	field_82905_a[1]->setTextureOffset(24, 22)->addBox(-4.0f, 4.0f, 0.5f, 11, 2, 2);
	field_82905_a[1]->setTextureOffset(24, 22)->addBox(-4.0f, 6.5f, 0.5f, 11, 2, 2);
	field_82905_a[2] = new ModelRenderer(this, 12, 22);
	field_82905_a[2]->addBox(0.0f, 0.0f, 0.0f, 3, 6, 3);

	field_82904_b[0] = new ModelRenderer(this, 0, 0);
	field_82904_b[0]->addBox(-4.0f, -4.0f, -4.0f, 8, 8, 8);
	field_82904_b[1] = new ModelRenderer(this, 32, 0);
	field_82904_b[1]->addBox(-4.0f, -4.0f, -4.0f, 6, 6, 6);
	field_82904_b[1]->rotationPointX = -8.0f;
	field_82904_b[1]->rotationPointY = 4.0f;
	field_82904_b[2] = new ModelRenderer(this, 32, 0);
	field_82904_b[2]->addBox(-4.0f, -4.0f, -4.0f, 6, 6, 6);
	field_82904_b[2]->rotationPointX = 10.0f;
	field_82904_b[2]->rotationPointY = 4.0f;
}

ModelWither::~ModelWither()
{
	for (ModelRenderer *part : field_82905_a)
		delete part;
	for (ModelRenderer *part : field_82904_b)
		delete part;
}

void ModelWither::render(Entity *entity, float f, float f1, float f2, float f3, float f4, float f5)
{
	(void)entity;
	setRotationAngles(f, f1, f2, f3, f4, f5);
	for (ModelRenderer *head : field_82904_b)
		head->render(f5);
	for (ModelRenderer *part : field_82905_a)
		part->render(f5);
}

void ModelWither::setRotationAngles(float f, float f1, float f2, float f3, float f4, float f5)
{
	(void)f; (void)f1; (void)f5;
	const float breath = MathHelper::cos(f2 * 0.1f);
	field_82905_a[1]->rotateAngleX = (0.065f + 0.05f * breath) * kPi;
	field_82905_a[2]->setRotationPoint(-2.0f,
		6.9f + MathHelper::cos(field_82905_a[1]->rotateAngleX) * 10.0f,
		-0.5f + MathHelper::sin(field_82905_a[1]->rotateAngleX) * 10.0f);
	field_82905_a[2]->rotateAngleX = (0.265f + 0.1f * breath) * kPi;
	field_82904_b[0]->rotateAngleY = f3 * kDegToRad;
	field_82904_b[0]->rotateAngleX = f4 * kDegToRad;
}

void ModelWither::setLivingAnimations(EntityLiving *entityliving, float f, float f1, float f2)
{
	(void)f; (void)f1; (void)f2;
	EntityWither *wither = dynamic_cast<EntityWither *>(entityliving);
	if (wither == nullptr)
		return;
	for (int_t head = 1; head < 3; ++head)
	{
		field_82904_b[head]->rotateAngleY = (wither->func_82207_a(head - 1) - entityliving->renderYawOffset) * kDegToRad;
		field_82904_b[head]->rotateAngleX = wither->func_82210_r(head - 1) * kDegToRad;
	}
}
