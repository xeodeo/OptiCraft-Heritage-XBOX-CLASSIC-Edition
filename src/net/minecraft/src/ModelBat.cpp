#include "ModelBat.h"

#include <cmath>

#include "EntityBat.h"
#include "MathHelper.h"
#include "ModelRenderer.h"

ModelBat::ModelBat()
{
	textureWidth = 64;
	textureHeight = 64;

	batHead = new ModelRenderer(this, 0, 0);
	batHead->addBox(-3.0f, -3.0f, -3.0f, 6, 6, 6);

	batRightEar = new ModelRenderer(this, 24, 0);
	batRightEar->addBox(-4.0f, -6.0f, -2.0f, 3, 4, 1);
	batHead->addChild(batRightEar);

	batLeftEar = new ModelRenderer(this, 24, 0);
	batLeftEar->mirror = true;
	batLeftEar->addBox(1.0f, -6.0f, -2.0f, 3, 4, 1);
	batHead->addChild(batLeftEar);

	batBody = new ModelRenderer(this, 0, 16);
	batBody->addBox(-3.0f, 4.0f, -3.0f, 6, 12, 6);
	batBody->setTextureOffset(0, 34)->addBox(-5.0f, 16.0f, 0.0f, 10, 6, 1);

	batRightWing = new ModelRenderer(this, 42, 0);
	batRightWing->addBox(-12.0f, 1.0f, 1.5f, 10, 16, 1);

	batOuterRightWing = new ModelRenderer(this, 24, 16);
	batOuterRightWing->setRotationPoint(-12.0f, 1.0f, 1.5f);
	batOuterRightWing->addBox(-8.0f, 1.0f, 0.0f, 8, 12, 1);

	batLeftWing = new ModelRenderer(this, 42, 0);
	batLeftWing->mirror = true;
	batLeftWing->addBox(2.0f, 1.0f, 1.5f, 10, 16, 1);

	batOuterLeftWing = new ModelRenderer(this, 24, 16);
	batOuterLeftWing->mirror = true;
	batOuterLeftWing->setRotationPoint(12.0f, 1.0f, 1.5f);
	batOuterLeftWing->addBox(0.0f, 1.0f, 0.0f, 8, 12, 1);

	batBody->addChild(batRightWing);
	batBody->addChild(batLeftWing);
	batRightWing->addChild(batOuterRightWing);
	batLeftWing->addChild(batOuterLeftWing);
}

ModelBat::~ModelBat()
{
	delete batHead;
	delete batRightEar;
	delete batLeftEar;
	delete batBody;
	delete batRightWing;
	delete batLeftWing;
	delete batOuterRightWing;
	delete batOuterLeftWing;
}

void ModelBat::render(Entity *entity, float f, float f1, float f2, float f3, float f4, float f5)
{
	(void)f;
	(void)f1;
	EntityBat *bat = dynamic_cast<EntityBat *>(entity);
	constexpr float radFactor = 57.295776f; // 180.0 / PI
	constexpr float pi = 3.1415927f;

	if (bat != nullptr && bat->getIsBatHanging())
	{
		batHead->rotateAngleX = f4 / radFactor;
		batHead->rotateAngleY = pi - f3 / radFactor;
		batHead->rotateAngleZ = pi;
		batHead->setRotationPoint(0.0f, -2.0f, 0.0f);
		batRightWing->setRotationPoint(-3.0f, 0.0f, 3.0f);
		batLeftWing->setRotationPoint(3.0f, 0.0f, 3.0f);
		batBody->rotateAngleX = pi;
		batRightWing->rotateAngleX = -pi / 20.0f;
		batRightWing->rotateAngleY = -pi * 2.0f / 5.0f;
		batOuterRightWing->rotateAngleY = -1.7278761f;
		batLeftWing->rotateAngleX = batRightWing->rotateAngleX;
		batLeftWing->rotateAngleY = -batRightWing->rotateAngleY;
		batOuterLeftWing->rotateAngleY = -batOuterRightWing->rotateAngleY;
	}
	else
	{
		batHead->rotateAngleX = f4 / radFactor;
		batHead->rotateAngleY = f3 / radFactor;
		batHead->rotateAngleZ = 0.0f;
		batHead->setRotationPoint(0.0f, 0.0f, 0.0f);
		batRightWing->setRotationPoint(0.0f, 0.0f, 0.0f);
		batLeftWing->setRotationPoint(0.0f, 0.0f, 0.0f);
		batBody->rotateAngleX = (pi / 4.0f) + MathHelper::cos(f2 * 0.1f) * 0.15f;
		batBody->rotateAngleY = 0.0f;
		batRightWing->rotateAngleY = MathHelper::cos(f2 * 1.3f) * pi * 0.25f;
		batLeftWing->rotateAngleY = -batRightWing->rotateAngleY;
		batOuterRightWing->rotateAngleY = batRightWing->rotateAngleY * 0.5f;
		batOuterLeftWing->rotateAngleY = -batRightWing->rotateAngleY * 0.5f;
	}

	batHead->render(f5);
	batBody->render(f5);
}
