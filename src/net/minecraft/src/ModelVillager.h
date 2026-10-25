#pragma once

#include "ModelBase.h"

class ModelRenderer;

// net.minecraft.src.ModelVillager
class ModelVillager : public ModelBase
{
public:
    explicit ModelVillager(float scale = 0.0f);
    ModelVillager(float scale, float yOffset);
    ModelVillager(float scale, float yOffset, int textureWidth, int textureHeight);
    ~ModelVillager() override;

    void render(float limbSwing, float limbAmount, float age, float headYaw, float headPitch, float scale) override;
    void setRotationAngles(float limbSwing, float limbAmount, float age, float headYaw, float headPitch, float scale) override;

    ModelRenderer* villagerHead;
    ModelRenderer* villagerBody;
    ModelRenderer* villagerArms;
    ModelRenderer* rightVillagerLeg;
    ModelRenderer* leftVillagerLeg;
    ModelRenderer* field_82898_f; // villagerNose

    // Legacy aliases
    ModelRenderer*& field_40340_a = villagerHead;
    ModelRenderer*& field_40338_b = villagerBody;
    ModelRenderer*& field_40339_c = villagerArms;
    ModelRenderer*& field_40336_d = rightVillagerLeg;
    ModelRenderer*& field_40337_e = leftVillagerLeg;
};
