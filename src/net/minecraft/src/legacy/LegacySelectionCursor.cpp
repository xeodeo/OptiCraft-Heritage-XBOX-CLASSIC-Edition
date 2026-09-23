#include "LegacySelectionCursor.h"

#include <algorithm>

#include "LegacyUiTexture.h"
#include "net/minecraft/src/Minecraft.h"
#include "net/minecraft/src/RenderEngine.h"
#include "platform/PlatformConfig.h"
#include "platform/PlatformTuning.h"

namespace
{
#if PLATFORM_PS2 || PLATFORM_WII || PLATFORM_XBOX
LegacyUiTexture g_selectionCursor("/cursor.png");
#endif
}

void legacyDrawSelectionCursorCentered(Minecraft *mc, int_t centerX, int_t centerY,
    int_t size, float_t zLevel)
{
#if PLATFORM_PS2 || PLATFORM_WII || PLATFORM_XBOX
    if (mc == nullptr || mc->renderEngine == nullptr || size <= 0)
        return;

    const int_t texture = g_selectionCursor.resolve(mc->renderEngine);
    if (texture < 0)
        return;

    legacyDrawUiTexture(texture, centerX - size / 2, centerY - size / 2,
        size, size, zLevel + 1.0f);
#else
    (void)mc;
    (void)centerX;
    (void)centerY;
    (void)size;
    (void)zLevel;
#endif
}

void legacyDrawSelectionCursor(Minecraft *mc, int_t controlX, int_t controlY,
    int_t controlHeight, float_t zLevel)
{
#if PLATFORM_PS2 || PLATFORM_WII || PLATFORM_XBOX
    const int_t size = std::max<int_t>(10, std::min<int_t>(PLATFORM_CURSOR_SIZE, controlHeight));
    legacyDrawSelectionCursorCentered(mc, controlX - 4, controlY + controlHeight / 2,
        size, zLevel);
#else
    (void)mc;
    (void)controlX;
    (void)controlY;
    (void)controlHeight;
    (void)zLevel;
#endif
}
