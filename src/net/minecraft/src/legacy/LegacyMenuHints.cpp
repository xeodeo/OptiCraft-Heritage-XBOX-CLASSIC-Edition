#include "LegacyMenuHints.h"
#include "net/minecraft/src/UiStrings.h"

#include <string>
#include <algorithm>

#include "net/minecraft/src/FontRenderer.h"
#include "platform/PlatformConfig.h"

#if PLATFORM_PS2
#include "LegacyButtonPrompt.h"
#include "net/minecraft/src/Minecraft.h"
#include "net/minecraft/src/RenderEngine.h"
#endif

void drawLegacyMenuHints(FontRenderer *font, int_t screenWidth, int_t screenHeight, bool showBack)
{
    if (font == nullptr)
        return;

#if PLATFORM_PS2
    Minecraft *mc = Minecraft::getMinecraft();
    RenderEngine *renderEngine = mc ? mc->renderEngine : nullptr;
    if (renderEngine != nullptr)
    {
        const int_t y = legacyHintRowY(screenHeight);
        const std::string navLabel = uiText("Navigate");
        const std::string selLabel = uiText("Select");
        const std::string backLabel = uiText("Back");

        const int_t wNav = LegacyButtonPrompt::getPromptWidth(font, Ps2ButtonIcon::DPad, navLabel);
        const int_t wSel = LegacyButtonPrompt::getPromptWidth(font, Ps2ButtonIcon::Cross, selLabel);
        const int_t wBack = showBack ? LegacyButtonPrompt::getPromptWidth(font, Ps2ButtonIcon::Circle, backLabel) : 0;

        const int_t gap = 16;
        const int_t totalWidth = wNav + gap + wSel + (showBack ? (gap + wBack) : 0);
        int_t x = std::max<int_t>(LEGACY_HINT_MARGIN, (screenWidth - totalWidth) / 2);

        x += LegacyButtonPrompt::drawPrompt(renderEngine, font, Ps2ButtonIcon::DPad, navLabel, x, y) + gap;
        x += LegacyButtonPrompt::drawPrompt(renderEngine, font, Ps2ButtonIcon::Cross, selLabel, x, y);
        if (showBack)
        {
            x += gap;
            LegacyButtonPrompt::drawPrompt(renderEngine, font, Ps2ButtonIcon::Circle, backLabel, x, y);
        }
        return;
    }
    const std::string navigate = "[D-Pad] " + uiText("Navigate");
    const std::string select = "[X] " + uiText("Select");
    const std::string back = "[O] " + uiText("Back");
#elif PLATFORM_WII || PLATFORM_XBOX
    const std::string navigate = "[D-Pad] " + uiText("Navigate");
    const std::string select = "[A] " + uiText("Select");
    const std::string back = "[B] " + uiText("Back");
#else
    const std::string navigate = "[Up/Down] " + uiText("Navigate");
    const std::string select = "[Enter] " + uiText("Select");
    const std::string back = "[Esc] " + uiText("Back");
#endif

    const int_t y = legacyHintRowY(screenHeight);
    const int_t columns = showBack ? 3 : 2;
    const int_t cell = std::max<int_t>(1, (screenWidth - LEGACY_HINT_MARGIN * 2) / columns);
    const std::string labels[] = {navigate, select, back};
    for (int_t i = 0; i < columns; ++i)
        font->drawStringWithShadow(font->trimStringToWidth(labels[i], cell - 2),
            LEGACY_HINT_MARGIN + i * cell, y, 0xf0f0f0);
}
