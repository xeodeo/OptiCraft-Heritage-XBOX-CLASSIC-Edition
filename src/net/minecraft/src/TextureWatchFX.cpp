#include "TextureWatchFX.h"

#include <cmath>
#include "Minecraft.h"
#include "Item.h"
#include "World.h"
#include "WorldProvider.h"
#include "RenderEngine.h"
#include "java/Math.h"
#include <algorithm>
#include <vector>

namespace
{
bool isWatchMagentaKey(int_t argb)
{
	const int_t r = (argb >> 16) & 0xff;
	const int_t g = (argb >> 8) & 0xff;
	const int_t b = argb & 0xff;
	return r == b && g == 0 && b > 0;
}

void sampleAtlasTile16(const std::vector<int_t> &atlas, int_t atlasW, int_t atlasH, int_t iconIndex, int_t outTile[256])
{
	std::fill(outTile, outTile + 256, 0);
	if (atlasW < 16 || atlasH < 16 || atlas.size() < static_cast<std::size_t>(atlasW) * static_cast<std::size_t>(atlasH))
		return;
	const int_t tileW = atlasW / 16;
	const int_t tileH = atlasH / 16;
	if (tileW <= 0 || tileH <= 0)
		return;
	const int_t startX = (iconIndex % 16) * tileW;
	const int_t startY = (iconIndex / 16) * tileH;
	for (int_t y = 0; y < 16; ++y)
	{
		const int_t sy = startY + y * tileH / 16;
		for (int_t x = 0; x < 16; ++x)
		{
			const int_t sx = startX + x * tileW / 16;
			outTile[y * 16 + x] = atlas[static_cast<std::size_t>(sy) * static_cast<std::size_t>(atlasW) + static_cast<std::size_t>(sx)];
		}
	}
}

void sampleImage16(const std::vector<int_t> &src, int_t srcW, int_t srcH, int_t outTile[256])
{
	std::fill(outTile, outTile + 256, 0);
	if (srcW <= 0 || srcH <= 0 || src.size() < static_cast<std::size_t>(srcW) * static_cast<std::size_t>(srcH))
		return;
	for (int_t y = 0; y < 16; ++y)
	{
		const int_t sy = y * srcH / 16;
		for (int_t x = 0; x < 16; ++x)
		{
			const int_t sx = x * srcW / 16;
			outTile[y * 16 + x] = src[static_cast<std::size_t>(sy) * static_cast<std::size_t>(srcW) + static_cast<std::size_t>(sx)];
		}
	}
}
} // namespace

TextureWatchFX::TextureWatchFX(Minecraft *minecraft) :
	TextureFX(Item::pocketSundial->getIconFromDamage(0)),
	mc(minecraft),
	stripFrameCount(0),
	stripLastFrame(-1),
	stripLastAnaglyph(false),
	compassFrame(0),
	compassAngle(0)
{
	tileImage = 1;
	std::fill(watchIconImageData, watchIconImageData + 256, 0);
	std::fill(dialImageData, dialImageData + 256, 0);
	if (mc != nullptr && mc->renderEngine != nullptr)
		reloadAssets(mc->renderEngine);
}

void TextureWatchFX::onTexturePackChanged(RenderEngine *renderengine)
{
	reloadAssets(renderengine);
}

void TextureWatchFX::reloadAssets(RenderEngine *renderengine)
{
	const bool wasStrip = stripFrameCount > 0;
	stripImageData.clear();
	stripFrameCount = 0;
	stripLastFrame = -1;
	std::fill(watchIconImageData, watchIconImageData + 256, 0);
	std::fill(dialImageData, dialImageData + 256, 0);

	if (renderengine == nullptr)
		return;

	// Mode 1: 1.5 / LCE animated vertical strip (/textures/items/clock.png).
	if (renderengine->hasResource("/textures/items/clock.png"))
	{
		std::vector<int_t> strip;
		int_t w = 0, h = 0;
		if (renderengine->readTextureImageDataWithSize("/textures/items/clock.png", strip, w, h) &&
		    w > 0 && h >= w * 2 && (h % w) == 0)
		{
			const int_t frames = h / w;
			stripImageData.resize(static_cast<std::size_t>(frames) * 256u);
			for (int_t f = 0; f < frames; ++f)
			{
				const int_t baseY = f * w;
				for (int_t y = 0; y < 16; ++y)
				{
					const int_t sy = baseY + y * w / 16;
					for (int_t x = 0; x < 16; ++x)
					{
						const int_t sx = x * w / 16;
						stripImageData[static_cast<std::size_t>(f) * 256u + static_cast<std::size_t>(y * 16 + x)] =
							strip[static_cast<std::size_t>(sy) * static_cast<std::size_t>(w) + static_cast<std::size_t>(sx)];
					}
				}
			}
			stripFrameCount = frames;
			if (!wasStrip)
			{
				compassFrame = 0.0;
				compassAngle = 0.0;
			}
			return;
		}
	}

	// Mode 2: 1.2.5 /gui/items.png magenta-masked icon + /misc/dial.png.
	if (wasStrip)
	{
		compassFrame = 0.0;
		compassAngle = 0.0;
	}

	std::vector<int_t> items;
	int_t itemsW = 0, itemsH = 0;
	if (renderengine->readTextureImageDataWithSize("/gui/items.png", items, itemsW, itemsH))
		sampleAtlasTile16(items, itemsW, itemsH, iconIndex, watchIconImageData);

	std::vector<int_t> dial;
	int_t dialW = 0, dialH = 0;
	if (renderengine->readTextureImageDataWithSize("/misc/dial.png", dial, dialW, dialH))
		sampleImage16(dial, dialW, dialH, dialImageData);

	// LCE / 1.5-era items.png sheets (including existing Legacy Console and DLC
	// packs without textures/items/clock.png) paint a static clock face with zero
	// magenta key pixels. Overlay the vanilla 1.2.5 magenta dial mask so the pack's
	// outer bezel is preserved while the sky dial still rotates smoothly.
	int_t magentaCount = 0;
	for (int_t i = 0; i < 256; ++i)
	{
		if (isWatchMagentaKey(watchIconImageData[i]))
			++magentaCount;
	}
	if (magentaCount == 0)
	{
		std::vector<int_t> defaultItems;
		int_t defW = 0, defH = 0;
		if (renderengine->readDefaultTextureImageDataWithSize("/gui/items.png", defaultItems, defW, defH))
		{
			int_t defTile[256];
			sampleAtlasTile16(defaultItems, defW, defH, iconIndex, defTile);
			for (int_t i = 0; i < 256; ++i)
			{
				if (isWatchMagentaKey(defTile[i]))
					watchIconImageData[i] = defTile[i];
			}
		}
	}
}

void TextureWatchFX::onTick()
{
	if (stripFrameCount > 0)
	{
		double d = 0.0;
		if (mc != nullptr && mc->theWorld != nullptr && mc->thePlayer != nullptr)
		{
			float f = mc->theWorld->getCelestialAngle(1.0f);
			d = static_cast<double>(f);
			if (mc->theWorld->worldProvider != nullptr && !mc->theWorld->worldProvider->func_48217_e())
				d = Math::random();
		}

		double d1 = d - compassFrame;
		while (d1 < -0.5) d1 += 1.0;
		while (d1 >= 0.5) d1 -= 1.0;
		if (d1 < -1.0) d1 = -1.0;
		if (d1 > 1.0) d1 = 1.0;

		compassAngle += d1 * 0.1;
		compassAngle *= 0.8;
		compassFrame += compassAngle;

		int_t frame = static_cast<int_t>((compassFrame + 1.0) * static_cast<double>(stripFrameCount)) % stripFrameCount;
		while (frame < 0)
			frame = (frame + stripFrameCount) % stripFrameCount;

		if (frame != stripLastFrame || anaglyphEnabled != stripLastAnaglyph)
		{
			stripLastFrame = frame;
			stripLastAnaglyph = anaglyphEnabled;
			const int_t *framePixels = &stripImageData[static_cast<std::size_t>(frame) * 256u];
			for (int_t i = 0; i < 256; ++i)
			{
				int_t a = (framePixels[i] >> 24) & 0xff;
				int_t r = (framePixels[i] >> 16) & 0xff;
				int_t g = (framePixels[i] >> 8) & 0xff;
				int_t b = framePixels[i] & 0xff;
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
		return;
	}

	double d = 0.0;
	if (mc->theWorld != nullptr && mc->thePlayer != nullptr)
	{
		float f = mc->theWorld->getCelestialAngle(1.0f);
		d = -f * 3.1415927f * 2.0f;
		if (mc->theWorld->worldProvider != nullptr && !mc->theWorld->worldProvider->func_48217_e())
		{
			d = Math::random() * 3.1415927410125732 * 2.0;
		}
	}

	double d1;
	for (d1 = d - compassFrame; d1 < -3.1415926535897931; d1 += 6.2831853071795862) {}
	for (; d1 >= 3.1415926535897931; d1 -= 6.2831853071795862) {}
	if (d1 < -1.0) d1 = -1.0;
	if (d1 > 1.0) d1 = 1.0;

	compassAngle += d1 * 0.10000000000000001;
	compassAngle *= 0.80000000000000004;
	compassFrame += compassAngle;

	const float d2 = sinf(static_cast<float>(compassFrame));
	const float d3 = cosf(static_cast<float>(compassFrame));

	for (int_t i = 0; i < 256; i++)
	{
		int_t j = (watchIconImageData[i] >> 24) & 0xff;
		int_t k = (watchIconImageData[i] >> 16) & 0xff;
		int_t l = (watchIconImageData[i] >> 8) & 0xff;
		int_t i1 = (watchIconImageData[i] >> 0) & 0xff;
		if (k == i1 && l == 0 && i1 > 0)
		{
			const float d4 = -(static_cast<float>(i % 16) / 15.0f - 0.5f);
			const float d5 = static_cast<float>(i / 16) / 15.0f - 0.5f;
			int_t i2 = k;
			int_t j2 = static_cast<int_t>((d4 * d3 + d5 * d2 + 0.5f) * 16.0f);
			int_t k2 = static_cast<int_t>(((d5 * d3 - d4 * d2) + 0.5f) * 16.0f);
			int_t l2 = (j2 & 0xf) + (k2 & 0xf) * 16;
			j = (dialImageData[l2] >> 24) & 0xff;
			k = ((dialImageData[l2] >> 16) & 0xff) * i2 / 255;
			l = ((dialImageData[l2] >> 8) & 0xff) * i2 / 255;
			i1 = ((dialImageData[l2] >> 0) & 0xff) * i2 / 255;
		}
		if (anaglyphEnabled)
		{
			int_t j1 = (k * 30 + l * 59 + i1 * 11) / 100;
			int_t k1 = (k * 30 + l * 70) / 100;
			int_t l1 = (k * 30 + i1 * 70) / 100;
			k = j1;
			l = k1;
			i1 = l1;
		}
		imageData[i * 4 + 0] = (byte_t)k;
		imageData[i * 4 + 1] = (byte_t)l;
		imageData[i * 4 + 2] = (byte_t)i1;
		imageData[i * 4 + 3] = (byte_t)j;
	}
}
