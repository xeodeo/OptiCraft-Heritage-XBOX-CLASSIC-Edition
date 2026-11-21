#pragma once

#include "TextureFX.h"
#include <vector>

class Minecraft;
class RenderEngine;

// net.minecraft.src.TextureCompassFX
class TextureCompassFX : public TextureFX
{
public:
	TextureCompassFX(Minecraft *minecraft);

	void onTick() override;
	void onTexturePackChanged(RenderEngine *renderengine) override;

private:
	void reloadAssets(RenderEngine *renderengine);

	Minecraft *mc;
	int_t compassIconImageData[256];
	std::vector<int_t> stripImageData;
	int_t stripFrameCount;
	int_t stripLastFrame;
	bool stripLastAnaglyph;
	double watchAngle; // angle
	double watchFrame; // angleDelta
};
