#pragma once

#include "net/minecraft/src/ModelBase.h"

class ModelRenderer;

// Original Minecraft-style blocky Chocobo model (clean-room box geometry)
class ModelChocobo : public ModelBase
{
public:
    ModelChocobo();
    ~ModelChocobo() override;

    void render(float f, float f1, float f2, float f3, float f4, float f5) override;
    void setRotationAngles(float f, float f1, float f2, float f3, float f4, float f5) override;

    bool showSaddle = false;
    bool showSaddlebags = false;

private:
    ModelRenderer *body;
    ModelRenderer *tail;
    ModelRenderer *neck;
    ModelRenderer *head;
    ModelRenderer *beak;
    ModelRenderer *crest;
    ModelRenderer *leftWing;
    ModelRenderer *rightWing;
    ModelRenderer *leftLeg;
    ModelRenderer *rightLeg;
    ModelRenderer *leftFoot;
    ModelRenderer *rightFoot;
    ModelRenderer *saddle;
    ModelRenderer *leftBag;
    ModelRenderer *rightBag;
};
