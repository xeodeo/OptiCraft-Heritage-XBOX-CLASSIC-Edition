#include "ScaledResolution.h"

#include <cmath>
#include "java/Arithmetic.h"
#include "GameSettings.h"
#include "client/Minecraft.h"
#include "platform/ConsoleAspectRatio.h"
#include "platform/PlatformTuning.h"

ScaledResolution::ScaledResolution(GameSettings *gamesettings, int_t i, int_t j)
{
	const bool widescreen = gamesettings != nullptr && gamesettings->widescreen;
	Minecraft *mc = Minecraft::getMinecraft();
	const bool verticalSplit = (mc != nullptr && mc->gameSettings != nullptr && mc->gameSettings->splitscreenVertical);
	const bool isSplit = (mc != nullptr && mc->isSplitScreenActive() && (j <= 256 || (verticalSplit && i <= 320)));

	if (isSplit && widescreen)
	{
		if (verticalSplit)
		{
			// In vertical split-screen (Left / Right), physical width (i) is half of the TV width.
			// Compute logical width from full TV width (i * 2) and halve it.
			scaledWidth = ConsoleAspectRatio::getLogicalWidth(i * 2, j, widescreen) / 2;
		}
		else
		{
			// In horizontal split-screen (Top / Bottom), physical height (j) is half of the TV display height (j * 2).
			// Anamorphic widescreen logical width must be computed from the full TV height,
			// otherwise the logical width is halved, squashing the HUD horizontally.
			scaledWidth = ConsoleAspectRatio::getLogicalWidth(i, j * 2, widescreen);
		}
	}
	else
	{
		scaledWidth = ConsoleAspectRatio::getLogicalWidth(i, j, widescreen);
	}
	scaledHeight = ConsoleAspectRatio::getLogicalHeight(j);
	scaleFactor = 1;
	exactScaleFactor = 1.0;
	if (gamesettings != nullptr && gamesettings->legacyUI && PLATFORM_LEGACY_GUI_SCALE > 0.0)
	{
		if (isSplit)
		{
			if (verticalSplit)
			{
				// In vertical split-screen (Left / Right), full vertical height (320 logical px) is available.
				exactScaleFactor = PLATFORM_LEGACY_GUI_SCALE * 0.70;
			}
			else
			{
				// In horizontal split-screen (Top / Bottom), vertical height is halved (~224 physical px).
				// 50% scale factor (1.0 exact scale) provides 224 logical height, comfortably fitting
				// the 222px tall double chest and 166px inventory without vertical clipping,
				// while rendering all GUI textures with 1:1 pixel perfection.
				exactScaleFactor = PLATFORM_LEGACY_GUI_SCALE * 0.50;
			}
		}
		else
		{
			exactScaleFactor = PLATFORM_LEGACY_GUI_SCALE;
		}
	}
	else
	{
#if PLATFORM_CONSOLE_LOW && PLATFORM_FORCE_GUI_SCALE > 0
		scaleFactor = PLATFORM_FORCE_GUI_SCALE;
		while (scaleFactor > 1 && (scaledWidth / scaleFactor < 1 || scaledHeight / scaleFactor < 1))
			scaleFactor--;
		exactScaleFactor = static_cast<double>(scaleFactor);
#else
		int_t k = gamesettings->guiScale;
		if (k == 0)
			k = 1000;
		for (; scaleFactor < k && scaledWidth / (scaleFactor + 1) >= 320 && scaledHeight / (scaleFactor + 1) >= 240; scaleFactor++)
		{
		}
		exactScaleFactor = static_cast<double>(scaleFactor);
#endif
	}
	field_25121_a = (double)scaledWidth / exactScaleFactor;
	field_25120_b = (double)scaledHeight / exactScaleFactor;
	scaledWidth = JavaArithmetic::doubleToInt(std::ceil(field_25121_a));
	scaledHeight = JavaArithmetic::doubleToInt(std::ceil(field_25120_b));
}

int_t ScaledResolution::getScaledWidth()
{
	return scaledWidth;
}

int_t ScaledResolution::getScaledHeight()
{
	return scaledHeight;
}


double ScaledResolution::getScaleFactorExact() const
{
	return exactScaleFactor;
}
