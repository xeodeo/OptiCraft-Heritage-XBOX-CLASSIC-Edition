#include "ModelPlayer.h"

#include "ModelRenderer.h"
#include "platform/RenderAPI.h"

namespace
{
ModelRenderer *makePart(ModelBase *model, int u, int v)
{
	ModelRenderer *part = new ModelRenderer(model, u, v);
	// UVs are baked when the box is added, so the size goes first.
	part->setTextureSize(64, 64);
	return part;
}

// 1.8 ModelBase.copyModelAngles: the wear layer follows its limb, pivot included.
void copyPose(const ModelRenderer *from, ModelRenderer *to)
{
	to->rotateAngleX = from->rotateAngleX;
	to->rotateAngleY = from->rotateAngleY;
	to->rotateAngleZ = from->rotateAngleZ;
	to->rotationPointX = from->rotationPointX;
	to->rotationPointY = from->rotationPointY;
	to->rotationPointZ = from->rotationPointZ;
}
}

ModelPlayer::ModelPlayer(float s, bool slim)
	: ModelBiped(s, 0.0f)
	, smallArms(slim)
{
	textureWidth = 64;
	textureHeight = 64;

	// ModelBiped built its parts for a 64x32 texture; rebuild the ones that
	// read the skin. Cloak (cape texture) and ears keep their 64x32 setup.
	delete bipedHead;
	delete bipedHeadwear;
	delete bipedBody;
	delete bipedRightArm;
	delete bipedLeftArm;
	delete bipedRightLeg;
	delete bipedLeftLeg;

	bipedHead = makePart(this, 0, 0);
	bipedHead->addBox(-4.0f, -8.0f, -4.0f, 8, 8, 8, s);
	bipedHead->setRotationPoint(0.0f, 0.0f, 0.0f);
	bipedHeadwear = makePart(this, 32, 0);
	bipedHeadwear->addBox(-4.0f, -8.0f, -4.0f, 8, 8, 8, s + 0.5f);
	bipedHeadwear->setRotationPoint(0.0f, 0.0f, 0.0f);

	bipedBody = makePart(this, 16, 16);
	bipedBody->addBox(-4.0f, 0.0f, -2.0f, 8, 12, 4, s);
	bipedBody->setRotationPoint(0.0f, 0.0f, 0.0f);
	bipedBodyWear = makePart(this, 16, 32);
	bipedBodyWear->addBox(-4.0f, 0.0f, -2.0f, 8, 12, 4, s + 0.25f);
	bipedBodyWear->setRotationPoint(0.0f, 0.0f, 0.0f);

	if (slim)
	{
		bipedLeftArm = makePart(this, 32, 48);
		bipedLeftArm->addBox(-1.0f, -2.0f, -2.0f, 3, 12, 4, s);
		bipedLeftArm->setRotationPoint(5.0f, 2.5f, 0.0f);
		bipedRightArm = makePart(this, 40, 16);
		bipedRightArm->addBox(-2.0f, -2.0f, -2.0f, 3, 12, 4, s);
		bipedRightArm->setRotationPoint(-5.0f, 2.5f, 0.0f);
		bipedLeftArmwear = makePart(this, 48, 48);
		bipedLeftArmwear->addBox(-1.0f, -2.0f, -2.0f, 3, 12, 4, s + 0.25f);
		bipedLeftArmwear->setRotationPoint(5.0f, 2.5f, 0.0f);
		bipedRightArmwear = makePart(this, 40, 32);
		bipedRightArmwear->addBox(-2.0f, -2.0f, -2.0f, 3, 12, 4, s + 0.25f);
		bipedRightArmwear->setRotationPoint(-5.0f, 2.5f, 10.0f);
	}
	else
	{
		bipedLeftArm = makePart(this, 32, 48);
		bipedLeftArm->addBox(-1.0f, -2.0f, -2.0f, 4, 12, 4, s);
		bipedLeftArm->setRotationPoint(5.0f, 2.0f, 0.0f);
		bipedRightArm = makePart(this, 40, 16);
		bipedRightArm->addBox(-3.0f, -2.0f, -2.0f, 4, 12, 4, s);
		bipedRightArm->setRotationPoint(-5.0f, 2.0f, 0.0f);
		bipedLeftArmwear = makePart(this, 48, 48);
		bipedLeftArmwear->addBox(-1.0f, -2.0f, -2.0f, 4, 12, 4, s + 0.25f);
		bipedLeftArmwear->setRotationPoint(5.0f, 2.0f, 0.0f);
		bipedRightArmwear = makePart(this, 40, 32);
		bipedRightArmwear->addBox(-3.0f, -2.0f, -2.0f, 4, 12, 4, s + 0.25f);
		bipedRightArmwear->setRotationPoint(-5.0f, 2.0f, 10.0f);
	}

	bipedRightLeg = makePart(this, 0, 16);
	bipedRightLeg->addBox(-2.0f, 0.0f, -2.0f, 4, 12, 4, s);
	bipedRightLeg->setRotationPoint(-1.9f, 12.0f, 0.0f);
	bipedLeftLeg = makePart(this, 16, 48);
	bipedLeftLeg->addBox(-2.0f, 0.0f, -2.0f, 4, 12, 4, s);
	bipedLeftLeg->setRotationPoint(1.9f, 12.0f, 0.0f);
	bipedRightLegwear = makePart(this, 0, 32);
	bipedRightLegwear->addBox(-2.0f, 0.0f, -2.0f, 4, 12, 4, s + 0.25f);
	bipedRightLegwear->setRotationPoint(-1.9f, 12.0f, 0.0f);
	bipedLeftLegwear = makePart(this, 0, 48);
	bipedLeftLegwear->addBox(-2.0f, 0.0f, -2.0f, 4, 12, 4, s + 0.25f);
	bipedLeftLegwear->setRotationPoint(1.9f, 12.0f, 0.0f);
}

ModelPlayer::~ModelPlayer()
{
	delete bipedBodyWear;
	delete bipedRightArmwear;
	delete bipedLeftArmwear;
	delete bipedRightLegwear;
	delete bipedLeftLegwear;
}

void ModelPlayer::copyWearPose()
{
	copyPose(bipedHead, bipedHeadwear);
	copyPose(bipedBody, bipedBodyWear);
	copyPose(bipedRightArm, bipedRightArmwear);
	copyPose(bipedLeftArm, bipedLeftArmwear);
	copyPose(bipedRightLeg, bipedRightLegwear);
	copyPose(bipedLeftLeg, bipedLeftLegwear);
}

void ModelPlayer::setRotationAngles(float f, float f1, float f2, float f3, float f4, float f5)
{
	ModelBiped::setRotationAngles(f, f1, f2, f3, f4, f5);
	copyWearPose();
}

void ModelPlayer::render(float f, float f1, float f2, float f3, float f4, float f5)
{
	setRotationAngles(f, f1, f2, f3, f4, f5);
	if (isChild)
	{
		const float half = 2.0f;
		renderPushMatrix();
		renderScale(1.5f / half, 1.5f / half, 1.5f / half);
		renderTranslate(0.0f, 16.0f * f5, 0.0f);
		bipedHead->render(f5);
		bipedHeadwear->render(f5);
		renderPopMatrix();
		renderPushMatrix();
		renderScale(1.0f / half, 1.0f / half, 1.0f / half);
		renderTranslate(0.0f, 24.0f * f5, 0.0f);
		bipedBody->render(f5);
		bipedRightArm->render(f5);
		bipedLeftArm->render(f5);
		bipedRightLeg->render(f5);
		bipedLeftLeg->render(f5);
		bipedLeftLegwear->render(f5);
		bipedRightLegwear->render(f5);
		bipedLeftArmwear->render(f5);
		bipedRightArmwear->render(f5);
		bipedBodyWear->render(f5);
		renderPopMatrix();
	}
	else
	{
		bipedHead->render(f5);
		bipedBody->render(f5);
		bipedRightArm->render(f5);
		bipedLeftArm->render(f5);
		bipedRightLeg->render(f5);
		bipedLeftLeg->render(f5);
		bipedHeadwear->render(f5);
		bipedLeftLegwear->render(f5);
		bipedRightLegwear->render(f5);
		bipedLeftArmwear->render(f5);
		bipedRightArmwear->render(f5);
		bipedBodyWear->render(f5);
	}
}

void ModelPlayer::renderRightArm(float scale)
{
	copyPose(bipedRightArm, bipedRightArmwear);
	bipedRightArm->render(scale);
	bipedRightArmwear->render(scale);
}
