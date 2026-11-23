#pragma once

#include "net/minecraft/src/ModelBase.h"
#include "net/minecraft/src/ModelRenderer.h"

// Port of ChocoCraft 4.1.5 (1.7.10) ModelChocobo (128x128 texture sheet)
class ModelChocobo : public ModelBase
{
public:
    ModelChocobo();
    ~ModelChocobo() override;

    void render(Entity *entity, float f, float f1, float f2, float f3, float f4, float f5) override;
    void setRotationAngles(float f, float f1, float f2, float f3, float f4, float f5) override;

    bool showSaddleBags;
    bool showPackBag;
    bool wingFlutter;
    bool isAirborne;

private:
    static void setRotation(ModelRenderer *model, float x, float y, float z);
    void setLeftLegXRotation(float deltaX);
    void setRightLegXRotation(float deltaX);

    ModelRenderer *head;
    ModelRenderer *feather;
    ModelRenderer *feather_2;
    ModelRenderer *neck;
    ModelRenderer *body;
    ModelRenderer *tail;
    ModelRenderer *tail_2;
    ModelRenderer *tail_3;
    ModelRenderer *rightWing;
    ModelRenderer *leftWing;
    ModelRenderer *rightThigh;
    ModelRenderer *leftThigh;
    ModelRenderer *rightShin;
    ModelRenderer *leftShin;
    ModelRenderer *talonRB;
    ModelRenderer *talonRR;
    ModelRenderer *talonRL;
    ModelRenderer *talonLL;
    ModelRenderer *talonLR;
    ModelRenderer *talonLB;
    ModelRenderer *rightSaddleBag;
    ModelRenderer *leftSaddleBag;
    ModelRenderer *packBag;
};

// Port of ChocoCraft 4.1.5 (1.7.10) ModelChicobo (64x32 texture sheet)
class ModelChicobo : public ModelBase
{
public:
    ModelChicobo();
    ~ModelChicobo() override;

    void render(Entity *entity, float f, float f1, float f2, float f3, float f4, float f5) override;
    void setRotationAngles(float f, float f1, float f2, float f3, float f4, float f5) override;

private:
    ModelRenderer *head;
    ModelRenderer *body;
    ModelRenderer *rightleg;
    ModelRenderer *leftleg;
};
