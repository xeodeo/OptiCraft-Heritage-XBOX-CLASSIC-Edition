#pragma once

#include <vector>
#include "TextureFX.h"

class Minecraft;
class RenderEngine;

// net.minecraft.src.TextureWatchFX
class TextureWatchFX : public TextureFX
{
public:
	TextureWatchFX(Minecraft *minecraft);

	void onTick() override;
	void onTexturePackChanged(RenderEngine *renderengine) override;

private:
	void reloadAssets(RenderEngine *renderengine);

	Minecraft *mc;
	int_t watchIconImageData[256];
	int_t dialImageData[256];
	std::vector<int_t> stripImageData;
	int_t stripFrameCount;
	int_t stripLastFrame;
	bool stripLastAnaglyph;
	double compassFrame; // angle (radians in dial mode, turns in strip mode)
	double compassAngle; // angleDelta
};
