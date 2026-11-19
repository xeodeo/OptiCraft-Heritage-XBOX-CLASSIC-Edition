#include "LegacyMenuHints.h"
#include "net/minecraft/src/ControlIcon.h"
#include "net/minecraft/src/HudSafeArea.h"
#include "net/minecraft/src/UiStrings.h"
#include "platform/PlatformConfig.h"

int_t legacyHintRowY(int_t screenHeight, bool splitScreen)
{
    const HudSafeInsets safe = computeHudSafeInsets(Minecraft::getMinecraft(), 0, screenHeight, splitScreen);
    const int_t baseOffset = splitScreen ? 12 : 15;
    return screenHeight - baseOffset - safe.bottom;
}

void drawLegacyMenuHints(Minecraft *mc, int_t screenWidth, int_t screenHeight, bool showBack)
{
#if PLATFORM_PS2
    const std::string buttons[] = {"D-Pad", "Cross", "Circle"};
#elif PLATFORM_WII || PLATFORM_XBOX
    const std::string buttons[] = {"D-Pad", "A", "B"};
#else
    const std::string buttons[] = {"Up/Down", "Enter", "Esc"};
#endif
    const std::string actions[] = {uiText("Navigate"), uiText("Select"), uiText("Back")};
    drawControlHintRow(mc, screenWidth, legacyHintRowY(screenHeight), buttons, actions, showBack ? 3 : 2);
}
