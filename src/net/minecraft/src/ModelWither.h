#pragma once

#include "ModelBase.h"

class ModelRenderer;
class EntityLiving;

// net.minecraft.src.ModelWither
class ModelWither : public ModelBase
{
public:
	ModelWither();
	~ModelWither() override;

	int_t func_82903_a() const { return 32; }
	void render(Entity *entity, float f, float f1, float f2, float f3, float f4, float f5) override;
	void setRotationAngles(float f, float f1, float f2, float f3, float f4, float f5) override;
	void setLivingAnimations(EntityLiving *entityliving, float f, float f1, float f2) override;

private:
	ModelRenderer *field_82905_a[3];   // spine, ribs, tail
	ModelRenderer *field_82904_b[3];   // centre head, side heads
};
