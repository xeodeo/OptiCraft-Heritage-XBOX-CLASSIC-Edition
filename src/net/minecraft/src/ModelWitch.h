#pragma once

#include "ModelVillager.h"

class ModelRenderer;
class Entity;

// net.minecraft.src.ModelWitch
class ModelWitch : public ModelVillager
{
public:
	explicit ModelWitch(float scale = 0.0f);
	~ModelWitch() override;

	void setRotationAngles(float limbSwing, float limbAmount, float age, float headYaw, float headPitch, float scale) override;
	void render(Entity *entity, float f, float f1, float f2, float f3, float f4, float f5) override;

	bool field_82900_g = false; // holding item / drinking

private:
	ModelRenderer *field_82901_h; // mole
	ModelRenderer *witchHat;
	ModelRenderer *hatTip1;
	ModelRenderer *hatTip2;
	ModelRenderer *hatTip3;
};
