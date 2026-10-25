#include "ModelZombieVillager.h"

#include "MathHelper.h"
#include "ModelRenderer.h"

ModelZombieVillager::ModelZombieVillager()
	: ModelZombieVillager(0.0f, 0.0f, false)
{
}

ModelZombieVillager::ModelZombieVillager(float f, float f1, bool isArmor)
	: ModelBiped(f, 0.0f, 64, isArmor ? 32 : 64)
{
	if (bipedHead != nullptr)
	{
		delete bipedHead;
	}

	if (isArmor)
	{
		bipedHead = new ModelRenderer(this, 0, 0);
		bipedHead->setTextureSize(textureWidth, textureHeight);
		bipedHead->addBox(-4.0f, -10.0f, -4.0f, 8, 6, 8, f);
		bipedHead->setRotationPoint(0.0f, 0.0f + f1, 0.0f);
	}
	else
	{
		bipedHead = new ModelRenderer(this);
		bipedHead->setTextureSize(textureWidth, textureHeight);
		bipedHead->setRotationPoint(0.0f, 0.0f + f1, 0.0f);
		bipedHead->setTextureOffset(0, 32)->addBox(-4.0f, -10.0f, -4.0f, 8, 10, 8, f);
		bipedHead->setTextureOffset(24, 32)->addBox(-1.0f, -3.0f, -6.0f, 2, 4, 2, f);
	}
}

void ModelZombieVillager::setRotationAngles(float f, float f1, float f2, float f3, float f4, float f5)
{
	ModelBiped::setRotationAngles(f, f1, f2, f3, f4, f5);
	float f6 = MathHelper::sin(onGround * 3.1415927f);
	float f7 = MathHelper::sin((1.0f - (1.0f - onGround) * (1.0f - onGround)) * 3.1415927f);
	bipedRightArm->rotateAngleZ = 0.0f;
	bipedLeftArm->rotateAngleZ = 0.0f;
	bipedRightArm->rotateAngleY = -(0.1f - f6 * 0.6f);
	bipedLeftArm->rotateAngleY = 0.1f - f6 * 0.6f;
	bipedRightArm->rotateAngleX = -1.5707964f;
	bipedLeftArm->rotateAngleX = -1.5707964f;
	bipedRightArm->rotateAngleX -= f6 * 1.2f - f7 * 0.4f;
	bipedLeftArm->rotateAngleX -= f6 * 1.2f - f7 * 0.4f;
	bipedRightArm->rotateAngleZ += MathHelper::cos(f2 * 0.09f) * 0.05f + 0.05f;
	bipedLeftArm->rotateAngleZ -= MathHelper::cos(f2 * 0.09f) * 0.05f + 0.05f;
	bipedRightArm->rotateAngleX += MathHelper::sin(f2 * 0.067f) * 0.05f;
	bipedLeftArm->rotateAngleX -= MathHelper::sin(f2 * 0.067f) * 0.05f;
}
