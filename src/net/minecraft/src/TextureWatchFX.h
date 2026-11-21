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
	std::vector<int_t> clockImageData;
	int_t frameCount;
	double field_94239_h;
	double field_94240_i;
	int_t frameCounter;
	bool lastAnaglyph;
};
