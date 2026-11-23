#include "ModelChocobo.h"
#include "EntityChocobo.h"

#include "net/minecraft/src/Entity.h"
#include "net/minecraft/src/MathHelper.h"

#include <cmath>

namespace
{
constexpr float kPi = 3.14159265358979323846f;
constexpr float kDegToRad = kPi / 180.0f;
} // namespace

// ============================================================================
// ModelChocobo (ChocoCraft 4.1.5 exact boxes, pivots, UVs, and animations)
// ============================================================================

ModelChocobo::ModelChocobo()
    : showSaddleBags(false)
    , showPackBag(false)
    , wingFlutter(true)
    , isAirborne(false)
{
    textureWidth = 128;
    textureHeight = 128;

    head = new ModelRenderer(this, 87, 2);
    neck = new ModelRenderer(this, 20, 45);
    feather = new ModelRenderer(this, 25, 18);
    feather_2 = new ModelRenderer(this, 45, 21);

    head->addBox(-3.0f, -12.0f, -9.0f, 6, 6, 12);
    head->setRotationPoint(1.0f, -8.0f, -5.0f);
    head->setTextureSize(textureWidth, textureHeight);
    head->mirror = true;
    setRotation(head, 0.0f, 0.0f, 0.0f);

    neck->addBox(-2.0f, -9.0f, -2.0f, 4, 14, 4);
    neck->setRotationPoint(1.0f, -8.0f, -5.0f);
    neck->setTextureSize(textureWidth, textureHeight);
    neck->mirror = true;
    setRotation(neck, 0.0f, 0.0f, 0.0f);

    feather->addBox(-3.0f, -12.0f, 3.0f, 6, 1, 5);
    feather->setRotationPoint(1.0f, -8.0f, -5.0f);
    feather->setTextureSize(textureWidth, textureHeight);
    feather->mirror = true;
    setRotation(feather, 0.1745329f, 0.0f, 0.0f);

    feather_2->addBox(-3.0f, -12.0f, 3.0f, 6, 6, 3);
    feather_2->setRotationPoint(1.0f, -8.0f, -5.0f);
    feather_2->setTextureSize(textureWidth, textureHeight);
    feather_2->mirror = true;
    setRotation(feather_2, 0.0f, 0.0f, 0.0f);

    body = new ModelRenderer(this, 77, 31);
    body->addBox(-3.0f, -4.0f, -3.0f, 12, 20, 11);
    body->setRotationPoint(-2.0f, 1.0f, -2.0f);
    body->setTextureSize(textureWidth, textureHeight);
    body->mirror = true;
    setRotation(body, 1.37881f, 0.0f, 0.0f);

    rightSaddleBag = new ModelRenderer(this, 2, 72);
    rightSaddleBag->addBox(-4.0f, 10.1f, 0.5f, 2, 6, 8);
    rightSaddleBag->setRotationPoint(-2.0f, 1.0f, -2.0f);
    rightSaddleBag->setTextureSize(textureWidth, textureHeight);
    rightSaddleBag->mirror = true;
    setRotation(rightSaddleBag, 1.37881f, 0.0f, 0.0f);

    leftSaddleBag = new ModelRenderer(this, 2, 96);
    leftSaddleBag->addBox(7.8f, 10.1f, 0.5f, 2, 6, 8);
    leftSaddleBag->setRotationPoint(-2.0f, 1.0f, -2.0f);
    leftSaddleBag->setTextureSize(textureWidth, textureHeight);
    leftSaddleBag->mirror = true;
    setRotation(leftSaddleBag, 1.37881f, 0.0f, 0.0f);

    packBag = new ModelRenderer(this, 50, 66);
    packBag->addBox(-2.0f, 3.0f, 8.0f, 10, 12, 6);
    packBag->setRotationPoint(-2.0f, 1.0f, -2.0f);
    packBag->setTextureSize(textureWidth, textureHeight);
    packBag->mirror = true;
    setRotation(packBag, 1.37881f, 0.0f, 0.0f);

    tail = new ModelRenderer(this, 59, 2);
    tail->addBox(0.0f, 0.0f, 0.0f, 12, 14, 1);
    tail->setRotationPoint(-5.0f, -11.0f, 26.6f);
    tail->setTextureSize(textureWidth, textureHeight);
    tail->mirror = true;
    setRotation(tail, -0.9948377f, 0.0f, 0.0f);

    tail_2 = new ModelRenderer(this, 31, 2);
    tail_2->addBox(0.0f, 0.0f, 0.0f, 12, 9, 1);
    tail_2->setRotationPoint(-5.0f, -4.0f, 23.0f);
    tail_2->setTextureSize(textureWidth, textureHeight);
    tail_2->mirror = true;
    setRotation(tail_2, -1.308997f, 0.0f, 0.0f);

    tail_3 = new ModelRenderer(this, 31, 2);
    tail_3->addBox(0.0f, 0.0f, 0.0f, 12, 9, 1);
    tail_3->setRotationPoint(-5.0f, -1.0f, 23.0f);
    tail_3->setTextureSize(textureWidth, textureHeight);
    tail_3->mirror = true;
    setRotation(tail_3, -1.37881f, 0.0f, 0.0f);

    rightWing = new ModelRenderer(this, 40, 37);
    leftWing = new ModelRenderer(this, 40, 37);

    rightWing->addBox(-1.0f, 0.0f, -3.0f, 1, 10, 16);
    rightWing->setRotationPoint(-5.0f, -7.0f, 0.0f);
    rightWing->setTextureSize(textureWidth, textureHeight);
    rightWing->mirror = true;
    setRotation(rightWing, 0.0f, -0.0174533f, 0.0f);

    leftWing->addBox(-1.0f, 0.0f, -3.0f, 1, 10, 16);
    leftWing->setRotationPoint(8.0f, -7.0f, 0.0f);
    leftWing->setTextureSize(textureWidth, textureHeight);
    leftWing->mirror = true;
    setRotation(leftWing, 0.0f, 0.0174533f, 0.0f);

    rightThigh = new ModelRenderer(this, 20, 27);
    rightShin = new ModelRenderer(this, 17, 2);
    talonRB = new ModelRenderer(this, 111, 22);
    talonRR = new ModelRenderer(this, 67, 20);
    talonRL = new ModelRenderer(this, 67, 20);

    rightThigh->addBox(-1.0f, 0.0f, -3.0f, 4, 9, 4);
    rightThigh->setRotationPoint(-4.0f, 4.0f, 5.0f);
    rightThigh->setTextureSize(textureWidth, textureHeight);
    rightThigh->mirror = true;

    rightShin->addBox(-1.0f, 8.0f, 1.0f, 3, 9, 3);
    rightShin->setRotationPoint(-4.0f, 4.0f, 5.0f);
    rightShin->setTextureSize(textureWidth, textureHeight);
    rightShin->mirror = true;

    talonRR->addBox(-1.0f, 14.0f, -14.0f, 2, 2, 7);
    talonRR->setRotationPoint(-4.0f, 4.0f, 5.0f);
    talonRR->setTextureSize(textureWidth, textureHeight);
    talonRR->mirror = true;

    talonRL->addBox(0.5f, 14.0f, -14.0f, 2, 2, 7);
    talonRL->setRotationPoint(-4.0f, 4.0f, 5.0f);
    talonRL->setTextureSize(textureWidth, textureHeight);
    talonRL->mirror = true;

    talonRB->addBox(-0.5f, 13.0f, 9.0f, 2, 2, 4);
    talonRB->setRotationPoint(-4.0f, 4.0f, 5.0f);
    talonRB->setTextureSize(textureWidth, textureHeight);
    talonRB->mirror = true;

    leftThigh = new ModelRenderer(this, 20, 27);
    leftShin = new ModelRenderer(this, 17, 2);
    talonLL = new ModelRenderer(this, 67, 20);
    talonLR = new ModelRenderer(this, 67, 20);
    talonLB = new ModelRenderer(this, 111, 22);

    leftThigh->addBox(-1.0f, 0.0f, -3.0f, 4, 9, 4);
    leftThigh->setRotationPoint(4.0f, 4.0f, 5.0f);
    leftThigh->setTextureSize(textureWidth, textureHeight);
    leftThigh->mirror = true;

    leftShin->addBox(0.0f, 8.0f, 1.0f, 3, 9, 3);
    leftShin->setRotationPoint(4.0f, 4.0f, 5.0f);
    leftShin->setTextureSize(textureWidth, textureHeight);
    leftShin->mirror = true;

    talonLR->addBox(0.0f, 14.0f, -14.0f, 2, 2, 7);
    talonLR->setRotationPoint(4.0f, 4.0f, 5.0f);
    talonLR->setTextureSize(textureWidth, textureHeight);
    talonLR->mirror = true;

    talonLL->addBox(1.5f, 14.0f, -14.0f, 2, 2, 7);
    talonLL->setRotationPoint(4.0f, 4.0f, 5.0f);
    talonLL->setTextureSize(textureWidth, textureHeight);
    talonLL->mirror = true;

    talonLB->addBox(0.5f, 13.0f, 9.0f, 2, 2, 4);
    talonLB->setRotationPoint(4.0f, 4.0f, 5.0f);
    talonLB->setTextureSize(textureWidth, textureHeight);
    talonLB->mirror = true;

    setRightLegXRotation(0.0f);
    setLeftLegXRotation(0.0f);
}

ModelChocobo::~ModelChocobo()
{
    delete head;
    delete feather;
    delete feather_2;
    delete neck;
    delete body;
    delete tail;
    delete tail_2;
    delete tail_3;
    delete rightWing;
    delete leftWing;
    delete rightThigh;
    delete leftThigh;
    delete rightShin;
    delete leftShin;
    delete talonRB;
    delete talonRR;
    delete talonRL;
    delete talonLL;
    delete talonLR;
    delete talonLB;
    delete rightSaddleBag;
    delete leftSaddleBag;
    delete packBag;
}

void ModelChocobo::setRotation(ModelRenderer *model, float x, float y, float z)
{
    model->rotateAngleX = x;
    model->rotateAngleY = y;
    model->rotateAngleZ = z;
}

void ModelChocobo::setLeftLegXRotation(float deltaX)
{
    setRotation(leftThigh, 0.2094395f + deltaX, 0.0f, 0.0f);
    setRotation(leftShin, -0.1919862f + deltaX, 0.0f, 0.0f);
    setRotation(talonLR, 0.3490659f + deltaX, 0.1570796f, 0.0f);
    setRotation(talonLL, 0.3490659f + deltaX, -0.1919862f, 0.0f);
    setRotation(talonLB, (-kPi / 6.0f) + deltaX, 0.0f, 0.0f);
}

void ModelChocobo::setRightLegXRotation(float deltaX)
{
    setRotation(rightThigh, 0.2094395f + deltaX, 0.0f, 0.0f);
    setRotation(rightShin, -0.1919862f + deltaX, 0.0f, 0.0f);
    setRotation(talonRR, 0.3490659f + deltaX, 0.1919862f, 0.0f);
    setRotation(talonRL, 0.3490659f + deltaX, -0.1570796f, 0.0f);
    setRotation(talonRB, (-kPi / 6.0f) + deltaX, 0.0f, 0.0f);
}

void ModelChocobo::setRotationAngles(float f, float f1, float f2, float f3, float f4, float f5)
{
    (void)f5;
    head->rotateAngleX = f4 * kDegToRad;
    head->rotateAngleY = f3 * kDegToRad;
    neck->rotateAngleX = 0.0f;
    neck->rotateAngleY = head->rotateAngleY;
    feather->rotateAngleX = head->rotateAngleX + 0.1745329f;
    feather->rotateAngleY = head->rotateAngleY;
    feather_2->rotateAngleX = head->rotateAngleX;
    feather_2->rotateAngleY = head->rotateAngleY;

    setRightLegXRotation(MathHelper::cos(f * 0.6662f) * 0.8f * f1);
    setLeftLegXRotation(MathHelper::cos(f * 0.6662f + kPi) * 0.8f * f1);

    if (isAirborne)
    {
        if (wingFlutter)
        {
            setRotation(rightWing, kPi * 0.5f - kPi / 12.0f, -0.0174533f, -f2);
            setRotation(leftWing, kPi * 0.5f - kPi / 12.0f, 0.0174533f, f2);
        }
        else
        {
            setRotation(rightWing, kPi * 0.5f, -0.0174533f, -1.2f);
            setRotation(leftWing, kPi * 0.5f, 0.0174533f, 1.2f);
        }
        setLeftLegXRotation(0.6f);
        setRightLegXRotation(0.6f);
    }
    else
    {
        setRotation(rightWing, 0.0f, -0.0174533f, 0.0f);
        setRotation(leftWing, 0.0f, 0.0174533f, 0.0f);
    }
}

void ModelChocobo::render(Entity *entity, float f, float f1, float f2, float f3, float f4, float f5)
{
    isAirborne = false;
    if (entity != nullptr)
    {
        EntityChocobo *choco = dynamic_cast<EntityChocobo *>(entity);
        isAirborne = !entity->onGround || std::fabs(entity->motionY) > 0.1 ||
                     (choco != nullptr && choco->isFlying());
    }
    setRotationAngles(f, f1, f2, f3, f4, f5);

    head->render(f5);
    feather->render(f5);
    feather_2->render(f5);
    neck->render(f5);
    body->render(f5);
    if (showSaddleBags)
    {
        leftSaddleBag->render(f5);
        rightSaddleBag->render(f5);
    }
    if (showPackBag)
    {
        packBag->render(f5);
    }
    rightWing->render(f5);
    leftWing->render(f5);
    tail->render(f5);
    tail_2->render(f5);
    tail_3->render(f5);
    rightThigh->render(f5);
    leftThigh->render(f5);
    rightShin->render(f5);
    leftShin->render(f5);
    talonRB->render(f5);
    talonRR->render(f5);
    talonRL->render(f5);
    talonLL->render(f5);
    talonLR->render(f5);
    talonLB->render(f5);
}

// ============================================================================
// ModelChicobo (ChocoCraft 4.1.5 baby chocobo, 64x32 texture sheet)
// ============================================================================

ModelChicobo::ModelChicobo()
{
    textureWidth = 64;
    textureHeight = 32;

    head = new ModelRenderer(this, 0, 0);
    head->addBox(-1.5f, -3.0f, -1.5f, 3, 3, 3, 0.0f);
    head->setRotationPoint(0.0f, 18.0f, -2.5f);

    body = new ModelRenderer(this, 0, 6);
    body->addBox(-2.0f, -2.5f, -2.0f, 4, 4, 4, 0.0f);
    body->setRotationPoint(0.0f, 20.0f, 0.0f);

    rightleg = new ModelRenderer(this, 12, 0);
    rightleg->addBox(-0.5f, 0.0f, -1.0f, 1, 2, 1, 0.0f);
    rightleg->setRotationPoint(-1.0f, 22.0f, 0.5f);

    leftleg = new ModelRenderer(this, 12, 0);
    leftleg->addBox(-0.5f, 0.0f, -1.0f, 1, 2, 1, 0.0f);
    leftleg->setRotationPoint(1.0f, 22.0f, 0.5f);
}

ModelChicobo::~ModelChicobo()
{
    delete head;
    delete body;
    delete rightleg;
    delete leftleg;
}

void ModelChicobo::setRotationAngles(float f, float f1, float f2, float f3, float f4, float f5)
{
    (void)f2;
    (void)f5;
    head->rotateAngleX = f4 * kDegToRad;
    head->rotateAngleY = f3 * kDegToRad;
    rightleg->rotateAngleX = MathHelper::cos(f * 0.6662f) * 1.4f * f1;
    leftleg->rotateAngleX = MathHelper::cos(f * 0.6662f + kPi) * 1.4f * f1;
}

void ModelChicobo::render(Entity *entity, float f, float f1, float f2, float f3, float f4, float f5)
{
    (void)entity;
    setRotationAngles(f, f1, f2, f3, f4, f5);
    head->render(f5);
    body->render(f5);
    rightleg->render(f5);
    leftleg->render(f5);
}
