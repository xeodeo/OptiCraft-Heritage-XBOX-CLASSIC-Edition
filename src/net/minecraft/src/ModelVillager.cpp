#include "ModelVillager.h"

#include "MathHelper.h"
#include "ModelRenderer.h"

ModelVillager::ModelVillager(float scale) : ModelVillager(scale, 0.0f, 64, 64)
{
}

ModelVillager::ModelVillager(float scale, float yOffset) : ModelVillager(scale, yOffset, 64, 64)
{
}

ModelVillager::ModelVillager(float scale, float yOffset, int texW, int texH)
{
    textureWidth = texW;
    textureHeight = texH;

    villagerHead = new ModelRenderer(this);
    villagerHead->setTextureSize(texW, texH);
    villagerHead->setRotationPoint(0.0f, yOffset, 0.0f);
    villagerHead->setTextureOffset(0, 0)->addBox(-4.0f, -10.0f, -4.0f, 8, 10, 8, scale);

    field_82898_f = new ModelRenderer(this);
    field_82898_f->setTextureSize(texW, texH);
    field_82898_f->setRotationPoint(0.0f, yOffset - 2.0f, 0.0f);
    field_82898_f->setTextureOffset(24, 0)->addBox(-1.0f, -1.0f, -6.0f, 2, 4, 2, scale);
    villagerHead->addChild(field_82898_f);

    villagerBody = new ModelRenderer(this);
    villagerBody->setTextureSize(texW, texH);
    villagerBody->setRotationPoint(0.0f, yOffset, 0.0f);
    villagerBody->setTextureOffset(16, 20)->addBox(-4.0f, 0.0f, -3.0f, 8, 12, 6, scale);
    villagerBody->setTextureOffset(0, 38)->addBox(-4.0f, 0.0f, -3.0f, 8, 18, 6, scale + 0.5f);

    villagerArms = new ModelRenderer(this);
    villagerArms->setTextureSize(texW, texH);
    villagerArms->setRotationPoint(0.0f, yOffset + 2.0f, 0.0f);
    villagerArms->setTextureOffset(44, 22)->addBox(-8.0f, -2.0f, -2.0f, 4, 8, 4, scale);
    villagerArms->setTextureOffset(44, 22)->addBox(4.0f, -2.0f, -2.0f, 4, 8, 4, scale);
    villagerArms->setTextureOffset(40, 38)->addBox(-4.0f, 2.0f, -2.0f, 8, 4, 4, scale);

    rightVillagerLeg = new ModelRenderer(this, 0, 22);
    rightVillagerLeg->setTextureSize(texW, texH);
    rightVillagerLeg->setRotationPoint(-2.0f, 12.0f + yOffset, 0.0f);
    rightVillagerLeg->addBox(-2.0f, 0.0f, -2.0f, 4, 12, 4, scale);

    leftVillagerLeg = new ModelRenderer(this, 0, 22);
    leftVillagerLeg->setTextureSize(texW, texH);
    leftVillagerLeg->mirror = true;
    leftVillagerLeg->setRotationPoint(2.0f, 12.0f + yOffset, 0.0f);
    leftVillagerLeg->addBox(-2.0f, 0.0f, -2.0f, 4, 12, 4, scale);
}

ModelVillager::~ModelVillager()
{
    delete villagerHead;
    delete field_82898_f;
    delete villagerBody;
    delete villagerArms;
    delete rightVillagerLeg;
    delete leftVillagerLeg;
}

void ModelVillager::render(float limbSwing, float limbAmount, float age, float headYaw, float headPitch, float scale)
{
    setRotationAngles(limbSwing, limbAmount, age, headYaw, headPitch, scale);
    villagerHead->render(scale);
    villagerBody->render(scale);
    rightVillagerLeg->render(scale);
    leftVillagerLeg->render(scale);
    villagerArms->render(scale);
}

void ModelVillager::setRotationAngles(float limbSwing, float limbAmount, float, float headYaw, float headPitch, float)
{
    constexpr float radFactor = 57.295776f;
    villagerHead->rotateAngleY = headYaw / radFactor;
    villagerHead->rotateAngleX = headPitch / radFactor;
    villagerArms->rotationPointY = 3.0f;
    villagerArms->rotationPointZ = -1.0f;
    villagerArms->rotateAngleX = -0.75f;
    rightVillagerLeg->rotateAngleX = MathHelper::cos(limbSwing * 0.6662f) * 1.4f * limbAmount * 0.5f;
    leftVillagerLeg->rotateAngleX = MathHelper::cos(limbSwing * 0.6662f + 3.1415927f) * 1.4f * limbAmount * 0.5f;
    rightVillagerLeg->rotateAngleY = 0.0f;
    leftVillagerLeg->rotateAngleY = 0.0f;
}
