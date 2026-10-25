#pragma once

#include "ModelBase.h"

class ModelRenderer;
class Entity;

// net.minecraft.src.ModelBat
class ModelBat : public ModelBase
{
public:
	ModelBat();
	~ModelBat() override;

	int getBatSize() const { return 36; }

	void render(Entity *entity, float f, float f1, float f2, float f3, float f4, float f5) override;

private:
	ModelRenderer *batHead;
	ModelRenderer *batRightEar;
	ModelRenderer *batLeftEar;
	ModelRenderer *batBody;
	ModelRenderer *batRightWing;
	ModelRenderer *batLeftWing;
	ModelRenderer *batOuterRightWing;
	ModelRenderer *batOuterLeftWing;
};
