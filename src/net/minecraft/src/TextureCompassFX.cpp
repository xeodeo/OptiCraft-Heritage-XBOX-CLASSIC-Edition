#include "TextureCompassFX.h"

#include <cmath>
#include "Minecraft.h"
#include "Item.h"
#include "World.h"
#include "ChunkCoordinates.h"
#include "EntityPlayerSP.h"
#include "WorldProvider.h"
#include "MathHelper.h"
#include "RenderEngine.h"
#include "java/Math.h"
#include <algorithm>

namespace
{
bool isStaticCompassRedPixel(int_t argb)
{
	const int_t a = (argb >> 24) & 0xff;
	const int_t r = (argb >> 16) & 0xff;
	const int_t g = (argb >> 8) & 0xff;
	const int_t b = argb & 0xff;
	return a > 0 && r >= 170 && g <= 65 && b <= 65;
}

void sampleCompassTile16(const std::vector<int_t> &atlas, int_t atlasW, int_t atlasH, int_t iconIndex, int_t outTile[256])
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
} // namespace

TextureCompassFX::TextureCompassFX(Minecraft *minecraft) :
	TextureFX(Item::compass->getIconFromDamage(0)),
	mc(minecraft),
	stripFrameCount(0),
	stripLastFrame(-1),
	stripLastAnaglyph(false),
	watchAngle(0),
	watchFrame(0)
{
	tileImage = 1;
	std::fill(compassIconImageData, compassIconImageData + 256, 0);
	if (mc != nullptr && mc->renderEngine != nullptr)
		reloadAssets(mc->renderEngine);
}

void TextureCompassFX::onTexturePackChanged(RenderEngine *renderengine)
{
	reloadAssets(renderengine);
}

void TextureCompassFX::reloadAssets(RenderEngine *renderengine)
{
	stripImageData.clear();
	stripFrameCount = 0;
	stripLastFrame = -1;
	std::fill(compassIconImageData, compassIconImageData + 256, 0);

	if (renderengine == nullptr)
		return;

	// Mode 1: 1.5 / LCE animated vertical strip (/textures/items/compass.png).
	if (renderengine->hasResource("/textures/items/compass.png"))
	{
		std::vector<int_t> strip;
		int_t w = 0, h = 0;
		if (renderengine->readTextureImageDataWithSize("/textures/items/compass.png", strip, w, h) &&
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
			return;
		}
	}

	// Mode 2: fallback /gui/items.png base face + procedural needle.
	std::vector<int_t> items;
	int_t itemsW = 0, itemsH = 0;
	if (renderengine->readTextureImageDataWithSize("/gui/items.png", items, itemsW, itemsH))
		sampleCompassTile16(items, itemsW, itemsH, iconIndex, compassIconImageData);

	for (int_t i = 0; i < 256; ++i)
	{
		if (isStaticCompassRedPixel(compassIconImageData[i]))
			compassIconImageData[i] = static_cast<int_t>(0xFF3A3A3A);
	}
}

void TextureCompassFX::onTick()
{
	double d = 0.0;
	if (mc != nullptr && mc->theWorld != nullptr && mc->thePlayer != nullptr)
	{
		ChunkCoordinates chunkcoordinates = mc->theWorld->getSpawnPoint();
		double d2 = (double)chunkcoordinates.x - mc->thePlayer->posX;
		double d4 = (double)chunkcoordinates.z - mc->thePlayer->posZ;
		d = ((double)(mc->thePlayer->rotationYaw - 90.0f) * 3.1415926535897931) / 180.0 - atan2(d4, d2);
		if (mc->theWorld->worldProvider != nullptr && !mc->theWorld->worldProvider->func_48217_e())
		{
			d = Math::random() * 3.1415927410125732 * 2.0;
		}
	}

	double d1;
	for (d1 = d - watchAngle; d1 < -3.1415926535897931; d1 += 6.2831853071795862) {}
	for (; d1 >= 3.1415926535897931; d1 -= 6.2831853071795862) {}
	if (d1 < -1.0) d1 = -1.0;
	if (d1 > 1.0) d1 = 1.0;

	watchFrame += d1 * 0.10000000000000001;
	watchFrame *= 0.80000000000000004;
	watchAngle += watchFrame;

	if (stripFrameCount > 0)
	{
		const double turns = -watchAngle / 6.2831853071795862;
		int_t frame = static_cast<int_t>((turns + 1.0) * static_cast<double>(stripFrameCount)) % stripFrameCount;
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

	for (int_t i = 0; i < 256; i++)
	{
		int_t j = (compassIconImageData[i] >> 24) & 0xff;
		int_t k = (compassIconImageData[i] >> 16) & 0xff;
		int_t l = (compassIconImageData[i] >> 8) & 0xff;
		int_t i1 = (compassIconImageData[i] >> 0) & 0xff;
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

	const float d3 = sinf(static_cast<float>(watchAngle));
	const float d5 = cosf(static_cast<float>(watchAngle));

	// Draw needle
	for (int_t i2 = -4; i2 <= 4; i2++)
	{
		int_t k2 = static_cast<int_t>(8.5f + d5 * static_cast<float>(i2) * 0.3f);
		int_t i3 = static_cast<int_t>(7.5f - d3 * static_cast<float>(i2) * 0.3f * 0.5f);
		int_t k3 = i3 * 16 + k2;
		int_t i4 = 100;
		int_t k4 = 100;
		int_t i5 = 100;
		byte_t c = (byte_t)0xff;
		if (anaglyphEnabled)
		{
			int_t k5 = (i4 * 30 + k4 * 59 + i5 * 11) / 100;
			int_t i6 = (i4 * 30 + k4 * 70) / 100;
			int_t k6 = (i4 * 30 + i5 * 70) / 100;
			i4 = k5;
			k4 = i6;
			i5 = k6;
		}
		imageData[k3 * 4 + 0] = (byte_t)i4;
		imageData[k3 * 4 + 1] = (byte_t)k4;
		imageData[k3 * 4 + 2] = (byte_t)i5;
		imageData[k3 * 4 + 3] = c;
	}

	for (int_t j2 = -8; j2 <= 16; j2++)
	{
		int_t l2 = static_cast<int_t>(8.5f + d3 * static_cast<float>(j2) * 0.3f);
		int_t j3 = static_cast<int_t>(7.5f + d5 * static_cast<float>(j2) * 0.3f * 0.5f);
		int_t l3 = j3 * 16 + l2;
		int_t j4 = j2 < 0 ? 100 : 255;
		int_t l4 = j2 < 0 ? 100 : 20;
		int_t j5 = j2 < 0 ? 100 : 20;
		byte_t c1 = (byte_t)0xff;
		if (anaglyphEnabled)
		{
			int_t l5 = (j4 * 30 + l4 * 59 + j5 * 11) / 100;
			int_t j6 = (j4 * 30 + l4 * 70) / 100;
			int_t l6 = (j4 * 30 + j5 * 70) / 100;
			j4 = l5;
			l4 = j6;
			j5 = l6;
		}
		imageData[l3 * 4 + 0] = (byte_t)j4;
		imageData[l3 * 4 + 1] = (byte_t)l4;
		imageData[l3 * 4 + 2] = (byte_t)j5;
		imageData[l3 * 4 + 3] = c1;
	}
}
