#include "ModelWitch.h"

#include <cmath>

#include "Entity.h"
#include "MathHelper.h"
#include "ModelRenderer.h"

ModelWitch::ModelWitch(float scale)
	: ModelVillager(scale, 0.0f, 64, 128)
{
	field_82901_h = new ModelRenderer(this);
	field_82901_h->setTextureSize(64, 128);
	field_82901_h->setRotationPoint(0.0f, -2.0f, 0.0f);
	field_82901_h->setTextureOffset(0, 0)->addBox(0.0f, 3.0f, -6.75f, 1, 1, 1, -0.25f);
	field_82898_f->addChild(field_82901_h);

	witchHat = new ModelRenderer(this);
	witchHat->setTextureSize(64, 128);
	witchHat->setRotationPoint(-5.0f, -10.03125f, -5.0f);
	witchHat->setTextureOffset(0, 64)->addBox(0.0f, 0.0f, 0.0f, 10, 2, 10);
	villagerHead->addChild(witchHat);

	hatTip1 = new ModelRenderer(this);
	hatTip1->setTextureSize(64, 128);
	hatTip1->setRotationPoint(1.75f, -4.0f, 2.0f);
	hatTip1->setTextureOffset(0, 76)->addBox(0.0f, 0.0f, 0.0f, 7, 4, 7);
	hatTip1->rotateAngleX = -0.05235988f;
	hatTip1->rotateAngleZ = 0.02617994f;
	witchHat->addChild(hatTip1);

	hatTip2 = new ModelRenderer(this);
	hatTip2->setTextureSize(64, 128);
	hatTip2->setRotationPoint(1.75f, -4.0f, 2.0f);
	hatTip2->setTextureOffset(0, 87)->addBox(0.0f, 0.0f, 0.0f, 4, 4, 4);
	hatTip2->rotateAngleX = -0.10471976f;
	hatTip2->rotateAngleZ = 0.05235988f;
	hatTip1->addChild(hatTip2);

	hatTip3 = new ModelRenderer(this);
	hatTip3->setTextureSize(64, 128);
	hatTip3->setRotationPoint(1.75f, -2.0f, 2.0f);
	hatTip3->setTextureOffset(0, 95)->addBox(0.0f, 0.0f, 0.0f, 1, 2, 1, 0.25f);
	hatTip3->rotateAngleX = -3.1415927f / 15.0f;
	hatTip3->rotateAngleZ = 0.10471976f;
	hatTip2->addChild(hatTip3);
}

ModelWitch::~ModelWitch()
{
	delete field_82901_h;
	delete witchHat;
	delete hatTip1;
	delete hatTip2;
	delete hatTip3;
}

void ModelWitch::setRotationAngles(float limbSwing, float limbAmount, float age, float headYaw, float headPitch, float scale)
{
	ModelVillager::setRotationAngles(limbSwing, limbAmount, age, headYaw, headPitch, scale);
}

void ModelWitch::render(Entity *entity, float limbSwing, float limbAmount, float age, float headYaw, float headPitch, float scale)
{
	ModelVillager::setRotationAngles(limbSwing, limbAmount, age, headYaw, headPitch, scale);

	if (field_82898_f != nullptr)
	{
		field_82898_f->offsetX = 0.0f;
		field_82898_f->offsetY = 0.0f;
		field_82898_f->offsetZ = 0.0f;

		if (entity != nullptr)
		{
			const float var8 = 0.01f * static_cast<float>(entity->entityId % 10);
			field_82898_f->rotateAngleX = MathHelper::sin(static_cast<float>(entity->ticksExisted) * var8) * 4.5f * 3.1415927f / 180.0f;
			field_82898_f->rotateAngleY = 0.0f;
			field_82898_f->rotateAngleZ = MathHelper::cos(static_cast<float>(entity->ticksExisted) * var8) * 2.5f * 3.1415927f / 180.0f;
		}

		if (field_82900_g)
		{
			field_82898_f->rotateAngleX = -0.9f;
			field_82898_f->offsetZ = -0.09375f;
			field_82898_f->offsetY = 0.1875f;
		}
	}

	villagerHead->render(scale);
	villagerBody->render(scale);
	rightVillagerLeg->render(scale);
	leftVillagerLeg->render(scale);
	villagerArms->render(scale);
}
