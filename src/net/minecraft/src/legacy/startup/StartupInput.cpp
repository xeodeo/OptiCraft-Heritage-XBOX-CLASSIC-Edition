#include "net/minecraft/src/legacy/startup/StartupInput.h"

#include "net/minecraft/src/legacy/startup/StartupInputPolicy.h"
#include "lwjgl/Display.h"
#include "platform/PlatformConfig.h"

#if PLATFORM_PC
#include "lwjgl/Keyboard.h"
#elif PLATFORM_PS2
#include "platform/Input.h"
#include "ps2/input/Ps2PadRuntime.h"
#else
#include "platform/Input.h"
#endif

namespace
{
bool s_previousSkipHeld = false;

bool pollSkipHeld()
{
#if PLATFORM_PC
    lwjgl::Display::processMessages();
    return lwjgl::Keyboard::isKeyDown(lwjgl::Keyboard::KEY_RETURN) ||
           lwjgl::Keyboard::isKeyDown(lwjgl::Keyboard::KEY_SPACE) ||
           lwjgl::Keyboard::isKeyDown(lwjgl::Keyboard::KEY_ESCAPE);
#elif PLATFORM_PS2
    // Avoid Display::processMessages() here: before MouseHelper exists the PS2
    // display backend would try to hand focus to gameplay. Poll only the pad.
    Ps2PadRuntime::poll();
    const PlatformTextInputSnapshot input = platformTextInputSnapshot(platformMenuPad());
    return LegacyStartup::shouldSkipForTextActions(input.held);
#else
    lwjgl::Display::processMessages();
    const PlatformTextInputSnapshot input = platformTextInputSnapshot(platformMenuPad());
    return LegacyStartup::shouldSkipForTextActions(input.held);
#endif
}
}

namespace LegacyStartup
{

void beginStartupInput()
{
    // Seed edge detection from the current held state so the button used to
    // confirm PS2 storage selection cannot also skip the first logo.
    s_previousSkipHeld = pollSkipHeld();
}

bool pollStartupSkipRequested()
{
    const bool held = pollSkipHeld();
    const bool pressed = held && !s_previousSkipHeld;
    s_previousSkipHeld = held;
    return pressed || lwjgl::Display::isCloseRequested();
}

} // namespace LegacyStartup
