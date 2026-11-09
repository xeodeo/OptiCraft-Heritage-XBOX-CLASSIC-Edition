#include "TextureWatchFX.h"

#include "Minecraft.h"
#include "Item.h"
#include "World.h"
#include "WorldProvider.h"
#include "RenderEngine.h"
#include "java/Math.h"

TextureWatchFX::TextureWatchFX(Minecraft *minecraft) :
	TextureFX(Item::pocketSundial->getIconFromDamage(0)),
	mc(minecraft),
	field_94239_h(0.0),
	field_94240_i(0.0),
	frameCounter(-1),
	lastAnaglyph(false)
{
	tileImage = 1;
	if (mc != nullptr && mc->renderEngine != nullptr && mc->renderEngine->hasResource("/textures/items/clock.png"))
	{
		clockImageData = mc->renderEngine->readTextureImageData("/textures/items/clock.png");
	}
}

void TextureWatchFX::onTick()
{
	const int_t frameCount = static_cast<int_t>(clockImageData.size() / 256u);
	if (frameCount <= 0)
		return;

	double d = 0.0;
	if (mc != nullptr && mc->theWorld != nullptr && mc->thePlayer != nullptr)
	{
		float f = mc->theWorld->getCelestialAngle(1.0f);
		d = static_cast<double>(f);
		if (mc->theWorld->worldProvider != nullptr && !mc->theWorld->worldProvider->func_48217_e())
		{
			d = Math::random();
		}
	}

	double d1 = d - field_94239_h;
	while (d1 < -0.5)
	{
		d1 += 1.0;
	}
	while (d1 >= 0.5)
	{
		d1 -= 1.0;
	}
	if (d1 < -1.0)
	{
		d1 = -1.0;
	}
	if (d1 > 1.0)
	{
		d1 = 1.0;
	}

	field_94240_i += d1 * 0.1;
	field_94240_i *= 0.8;
	field_94239_h += field_94240_i;

	int_t frame = static_cast<int_t>((field_94239_h + 1.0) * static_cast<double>(frameCount)) % frameCount;
	while (frame < 0)
	{
		frame = (frame + frameCount) % frameCount;
	}

	if (frame != frameCounter || anaglyphEnabled != lastAnaglyph)
	{
		frameCounter = frame;
		lastAnaglyph = anaglyphEnabled;
		const int_t *framePixels = &clockImageData[static_cast<std::size_t>(frameCounter) * 256u];
		for (int_t i = 0; i < 256; ++i)
		{
			int_t a = (framePixels[i] >> 24) & 0xff;
			int_t r = (framePixels[i] >> 16) & 0xff;
			int_t g = (framePixels[i] >> 8) & 0xff;
			int_t b = (framePixels[i] >> 0) & 0xff;
			if (anaglyphEnabled)
			{
				int_t nr = (r * 30 + g * 59 + b * 11) / 100;
				int_t ng = (r * 30 + g * 70) / 100;
				int_t nb = (r * 30 + b * 70) / 100;
				r = nr;
				g = ng;
				b = nb;
			}
			imageData[i * 4 + 0] = static_cast<byte_t>(r);
			imageData[i * 4 + 1] = static_cast<byte_t>(g);
			imageData[i * 4 + 2] = static_cast<byte_t>(b);
			imageData[i * 4 + 3] = static_cast<byte_t>(a);
		}
	}
}
