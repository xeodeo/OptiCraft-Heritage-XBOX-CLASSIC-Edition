#pragma once

#include <vector>
#include "TextureFX.h"

class Minecraft;

// net.minecraft.src.TextureWatchFX
class TextureWatchFX : public TextureFX
{
public:
	TextureWatchFX(Minecraft *minecraft);

	void onTick() override;

private:
	Minecraft *mc;
	std::vector<int_t> clockImageData;
	double field_94239_h;
	double field_94240_i;
	int_t frameCounter;
	bool lastAnaglyph;
};
