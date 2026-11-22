#pragma once

#include "ModelBiped.h"

// Minecraft 1.8 net.minecraft.client.model.ModelPlayer: the 64x64 skin
// layout with a second ("wear") layer on every body part, separate left
// arm/leg texture areas and optional slim (3 px, "Alex") arms.
// Only players drawn with a 64x64 skin use it; mobs and 64x32 skins keep
// the plain ModelBiped.
class ModelPlayer : public ModelBiped
{
public:
	ModelPlayer(float scale, bool smallArms);
	~ModelPlayer() override;

	void render(float f, float f1, float f2, float f3, float f4, float f5) override;
	void setRotationAngles(float f, float f1, float f2, float f3, float f4, float f5) override;

	// First-person hand: right arm plus its sleeve layer.
	void renderRightArm(float scale);

	ModelRenderer *bipedBodyWear;
	ModelRenderer *bipedRightArmwear;
	ModelRenderer *bipedLeftArmwear;
	ModelRenderer *bipedRightLegwear;
	ModelRenderer *bipedLeftLegwear;
	bool smallArms;

private:
	void copyWearPose();
};
