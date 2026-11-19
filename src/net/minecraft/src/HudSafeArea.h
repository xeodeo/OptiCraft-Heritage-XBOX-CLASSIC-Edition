#pragma once

#include <algorithm>
#include "java/Type.h"
#include "Gui.h"
#include "GameSettings.h"
#include "client/Minecraft.h"
#include "platform/PlatformConfig.h"

struct HudSafeInsets
{
	int_t left = 0;
	int_t top = 0;
	int_t right = 0;
	int_t bottom = 0;
};

inline int_t hudSafeAreaDefaultPercent()
{
#if PLATFORM_XBOX
	return 4;
#else
	return 0;
#endif
}

inline int_t hudSafeAreaPercent(const GameSettings *settings)
{
	if (settings == nullptr)
		return hudSafeAreaDefaultPercent();
	if (settings->hudSafeArea < 0)
		return 0;
	if (settings->hudSafeArea > 10)
		return 10;
	return settings->hudSafeArea;
}

inline int_t hudSafeMarginPixels(int_t extent, int_t percent)
{
	if (extent <= 0 || percent <= 0)
		return 0;
	return (extent * percent + 50) / 100;
}

inline HudSafeInsets computeHudSafeInsets(Minecraft *mc, int_t sw, int_t sh, bool applySplitOuterEdgesOnly = true)
{
	HudSafeInsets out;
	const GameSettings *settings = mc != nullptr ? mc->gameSettings : nullptr;
	const int_t pct = hudSafeAreaPercent(settings);
	if (pct <= 0)
		return out;

	const bool isSplit = applySplitOuterEdgesOnly && mc != nullptr && mc->isSplitScreenActive();
	const bool vertical = isSplit && settings != nullptr && settings->splitscreenVertical;
	const int_t fullSw = (isSplit && vertical) ? sw * 2 : sw;
	const int_t fullSh = (isSplit && !vertical) ? sh * 2 : sh;

	int_t mx = hudSafeMarginPixels(fullSw, pct);
	int_t my = hudSafeMarginPixels(fullSh, pct);
	if (sw > 0 && mx > sw / 5)
		mx = sw / 5;
	if (sh > 0 && my > sh / 5)
		my = sh / 5;

	if (!isSplit)
	{
		out.left = mx;
		out.right = mx;
		out.top = my;
		out.bottom = my;
		return out;
	}

	const bool isP2 = (mc->thePlayer != nullptr && mc->thePlayer == mc->thePlayer2);
	if (vertical)
	{
		out.left = isP2 ? 0 : mx;
		out.right = isP2 ? mx : 0;
		out.top = my;
		out.bottom = my;
	}
	else
	{
		out.left = mx;
		out.right = mx;
		out.top = isP2 ? 0 : my;
		out.bottom = isP2 ? my : 0;
	}
	return out;
}

// Draws crisp L-shaped corner brackets showing the active HUD safe area while
// the user adjusts the Safe Area slider in Video Options.
inline void drawHudSafeAreaPreview(int_t screenWidth, int_t screenHeight, int_t percent)
{
	if (screenWidth <= 16 || screenHeight <= 16)
		return;
	if (percent < 0) percent = 0;
	if (percent > 10) percent = 10;
	const int_t mx = hudSafeMarginPixels(screenWidth, percent);
	const int_t my = hudSafeMarginPixels(screenHeight, percent);
	const int_t x0 = mx;
	const int_t y0 = my;
	const int_t x1 = screenWidth - mx;
	const int_t y1 = screenHeight - my;
	const int_t arm = 12;
	const int_t shadow = static_cast<int_t>(0xc0000000u);
	const int_t bright = static_cast<int_t>(0xfffff060u);

	// Outer dim frame so the clipped/safe margin is unmistakable on any TV.
	if (my > 0)
	{
		Gui::drawRect(0, 0, screenWidth, y0, static_cast<int_t>(0x38000000u));
		Gui::drawRect(0, y1, screenWidth, screenHeight, static_cast<int_t>(0x38000000u));
	}
	if (mx > 0)
	{
		Gui::drawRect(0, y0, x0, y1, static_cast<int_t>(0x38000000u));
		Gui::drawRect(x1, y0, screenWidth, y1, static_cast<int_t>(0x38000000u));
	}

	// Top-left bracket
	Gui::drawRect(x0, y0, x0 + arm + 1, y0 + 2, shadow);
	Gui::drawRect(x0, y0, x0 + 2, y0 + arm + 1, shadow);
	Gui::drawRect(x0, y0, x0 + arm, y0 + 1, bright);
	Gui::drawRect(x0, y0, x0 + 1, y0 + arm, bright);

	// Top-right bracket
	Gui::drawRect(x1 - arm - 1, y0, x1, y0 + 2, shadow);
	Gui::drawRect(x1 - 2, y0, x1, y0 + arm + 1, shadow);
	Gui::drawRect(x1 - arm, y0, x1, y0 + 1, bright);
	Gui::drawRect(x1 - 1, y0, x1, y0 + arm, bright);

	// Bottom-left bracket
	Gui::drawRect(x0, y1 - 2, x0 + arm + 1, y1, shadow);
	Gui::drawRect(x0, y1 - arm - 1, x0 + 2, y1, shadow);
	Gui::drawRect(x0, y1 - 1, x0 + arm, y1, bright);
	Gui::drawRect(x0, y1 - arm, x0 + 1, y1, bright);

	// Bottom-right bracket
	Gui::drawRect(x1 - arm - 1, y1 - 2, x1, y1, shadow);
	Gui::drawRect(x1 - 2, y1 - arm - 1, x1, y1, shadow);
	Gui::drawRect(x1 - arm, y1 - 1, x1, y1, bright);
	Gui::drawRect(x1 - 1, y1 - arm, x1, y1, bright);
}
