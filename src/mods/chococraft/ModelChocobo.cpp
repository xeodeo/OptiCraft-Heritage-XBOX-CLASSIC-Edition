#include "ModelChocobo.h"

#include "net/minecraft/src/ModelRenderer.h"
#include "net/minecraft/src/MathHelper.h"
#include "platform/RenderAPI.h"

ModelChocobo::ModelChocobo()
{
    // Original 64x32 texture atlas regions:
    //   body: (0, 12), tail: (36, 18), neck: (24, 0), head: (0, 0),
    //   beak: (42, 0), crest: (52, 0), wings: (36, 0), legs: (52, 10),
    //   saddle/bags: (0, 24) / (28, 22)

    body = new ModelRenderer(0, 12);
    body->addBox(-5.0f, -4.0f, -6.0f, 10, 8, 12, 0.0f);
    body->setRotationPoint(0.0f, 9.0f, 0.0f);

    tail = new ModelRenderer(36, 18);
    tail->addBox(-4.0f, -2.0f, 0.0f, 8, 4, 6, 0.0f);
    tail->setRotationPoint(0.0f, 7.5f, 5.5f);

    neck = new ModelRenderer(24, 0);
    neck->addBox(-2.0f, -8.0f, -2.0f, 4, 9, 4, 0.0f);
    neck->setRotationPoint(0.0f, 6.5f, -5.0f);

    head = new ModelRenderer(0, 0);
    head->addBox(-3.0f, -6.0f, -3.5f, 6, 6, 6, 0.0f);
    head->setRotationPoint(0.0f, -1.5f, -5.5f);

    beak = new ModelRenderer(42, 0);
    beak->addBox(-2.0f, -3.0f, -6.5f, 4, 2, 3, 0.0f);
    beak->setRotationPoint(0.0f, -1.5f, -5.5f);

    crest = new ModelRenderer(52, 0);
    crest->addBox(-1.5f, -8.0f, -0.5f, 3, 3, 5, 0.0f);
    crest->setRotationPoint(0.0f, -1.5f, -5.5f);

    rightWing = new ModelRenderer(36, 6);
    rightWing->addBox(-1.0f, 0.0f, -4.5f, 1, 6, 9, 0.0f);
    rightWing->setRotationPoint(-5.0f, 6.0f, 0.0f);

    leftWing = new ModelRenderer(36, 6);
    leftWing->addBox(0.0f, 0.0f, -4.5f, 1, 6, 9, 0.0f);
    leftWing->setRotationPoint(5.0f, 6.0f, 0.0f);

    rightLeg = new ModelRenderer(52, 10);
    rightLeg->addBox(-1.5f, 0.0f, -1.5f, 3, 10, 3, 0.0f);
    rightLeg->setRotationPoint(-2.8f, 13.0f, 0.5f);

    leftLeg = new ModelRenderer(52, 10);
    leftLeg->addBox(-1.5f, 0.0f, -1.5f, 3, 10, 3, 0.0f);
    leftLeg->setRotationPoint(2.8f, 13.0f, 0.5f);

    rightFoot = new ModelRenderer(42, 24);
    rightFoot->addBox(-2.0f, 9.5f, -3.5f, 4, 1, 5, 0.0f);
    rightFoot->setRotationPoint(-2.8f, 13.0f, 0.5f);

    leftFoot = new ModelRenderer(42, 24);
    leftFoot->addBox(-2.0f, 9.5f, -3.5f, 4, 1, 5, 0.0f);
    leftFoot->setRotationPoint(2.8f, 13.0f, 0.5f);

    // Optional saddle & saddlebags
    saddle = new ModelRenderer(0, 24);
    saddle->addBox(-5.3f, -4.4f, -3.5f, 10, 2, 7, 0.25f);
    saddle->setRotationPoint(0.0f, 9.0f, 0.0f);

    rightBag = new ModelRenderer(28, 22);
    rightBag->addBox(-7.2f, -2.0f, -2.5f, 2, 5, 6, 0.0f);
    rightBag->setRotationPoint(0.0f, 9.0f, 1.0f);

    leftBag = new ModelRenderer(28, 22);
    leftBag->addBox(5.2f, -2.0f, -2.5f, 2, 5, 6, 0.0f);
    leftBag->setRotationPoint(0.0f, 9.0f, 1.0f);
}

ModelChocobo::~ModelChocobo()
{
    delete body;
    delete tail;
    delete neck;
    delete head;
    delete beak;
    delete crest;
    delete leftWing;
    delete rightWing;
    delete leftLeg;
    delete rightLeg;
    delete leftFoot;
    delete rightFoot;
    delete saddle;
    delete leftBag;
    delete rightBag;
}

void ModelChocobo::setRotationAngles(float limbSwing, float limbAmount, float wingAngle, float headYaw, float headPitch, float scale)
{
    (void)scale;
    const float yawRad = headYaw / 57.295776f;
    const float pitchRad = headPitch / 57.295776f;

    head->rotateAngleY = yawRad;
    head->rotateAngleX = pitchRad;
    beak->rotateAngleY = yawRad;
    beak->rotateAngleX = pitchRad;
    crest->rotateAngleY = yawRad;
    crest->rotateAngleX = pitchRad - 0.2f;

    neck->rotateAngleX = 0.15f;
    tail->rotateAngleX = 0.45f;

    const float legSwing = MathHelper::cos(limbSwing * 0.6662f) * 1.15f * limbAmount;
    rightLeg->rotateAngleX = legSwing;
    rightFoot->rotateAngleX = legSwing;
    leftLeg->rotateAngleX = -legSwing;
    leftFoot->rotateAngleX = -legSwing;

    rightWing->rotateAngleZ = wingAngle;
    leftWing->rotateAngleZ = -wingAngle;
}

void ModelChocobo::render(float limbSwing, float limbAmount, float wingAngle, float headYaw, float headPitch, float scale)
{
    setRotationAngles(limbSwing, limbAmount, wingAngle, headYaw, headPitch, scale);

    if (isChild)
    {
        // Cute Chicobo proportions
        renderPushMatrix();
        renderScale(0.55f, 0.55f, 0.55f);
        renderTranslate(0.0f, 19.5f * scale, 0.0f);
        body->render(scale);
        tail->render(scale);
        neck->render(scale);
        rightWing->render(scale);
        leftWing->render(scale);
        rightLeg->render(scale);
        leftLeg->render(scale);
        rightFoot->render(scale);
        leftFoot->render(scale);
        renderPopMatrix();

        renderPushMatrix();
        renderScale(0.72f, 0.72f, 0.72f);
        renderTranslate(0.0f, 12.0f * scale, 1.0f * scale);
        head->render(scale);
        beak->render(scale);
        crest->render(scale);
        renderPopMatrix();
    }
    else
    {
        body->render(scale);
        tail->render(scale);
        neck->render(scale);
        head->render(scale);
        beak->render(scale);
        crest->render(scale);
        rightWing->render(scale);
        leftWing->render(scale);
        rightLeg->render(scale);
        leftLeg->render(scale);
        rightFoot->render(scale);
        leftFoot->render(scale);
        if (showSaddle)
            saddle->render(scale);
        if (showSaddlebags)
        {
            rightBag->render(scale);
            leftBag->render(scale);
        }
    }
}
