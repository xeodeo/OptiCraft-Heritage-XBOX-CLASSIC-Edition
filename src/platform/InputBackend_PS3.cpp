// PS3 implementation of platform/Input.h, over src/ps3/input (same shape as
// InputBackend_XBOX.cpp: DualShock 3 controller, stick-driven menu cursor).
#include "platform/Input.h"
#include "platform/Log.h"

#include "ps3/input/Ps3Input.h"
#include "ps3/input/Ps3Pad.h"

namespace
{
std::uint32_t mapTextButtons(unsigned short bits)
{
    std::uint32_t value = 0;
    if (bits & PS3_PAD_DPAD_LEFT) value |= PLATFORM_TEXT_LEFT;
    if (bits & PS3_PAD_DPAD_RIGHT) value |= PLATFORM_TEXT_RIGHT;
    if (bits & PS3_PAD_DPAD_UP) value |= PLATFORM_TEXT_UP;
    if (bits & PS3_PAD_DPAD_DOWN) value |= PLATFORM_TEXT_DOWN;
    if (bits & PS3_PAD_CROSS) value |= PLATFORM_TEXT_TYPE;
    if (bits & PS3_PAD_SQUARE) value |= PLATFORM_TEXT_BACK;
    if (bits & PS3_PAD_SELECT) value |= PLATFORM_TEXT_SPACE;
    if (bits & PS3_PAD_TRIANGLE) value |= PLATFORM_TEXT_SHIFT;
    if (bits & PS3_PAD_START) value |= PLATFORM_TEXT_ENTER;
    if (bits & PS3_PAD_CIRCLE) value |= PLATFORM_TEXT_CLOSE;
    if (bits & PS3_PAD_L1) value |= PLATFORM_TEXT_TAB_LEFT;
    if (bits & PS3_PAD_R1) value |= PLATFORM_TEXT_TAB_RIGHT;
    return value;
}
}

PlatformTextInputSnapshot platformTextInputSnapshot(int port)
{
    (void)port;
    PlatformTextInputSnapshot out;
    // The controller of the player whose screen is being run (split screen).
    const Ps3PadSnapshot& pad = Ps3Pad::playerSnapshot(Ps3Pad::menuPlayer());
    out.connected = pad.connected;
    out.held = mapTextButtons(pad.held);
    out.pressed = mapTextButtons(Ps3Pad::consumePressed());
#if MC_LOG_LEVEL > 0
    if (out.pressed != 0)
        MC_LOG_INFO("ps3.input", "text pressed=%04x held=%04x\n", out.pressed, out.held);
#endif
    return out;
}

PlatformGamepadSnapshot platformGamepadSnapshot(int port)
{
    // Port 1 is split screen's player 2; port 0 follows the menu routing.
    PlatformGamepadSnapshot out;
    const Ps3PadSnapshot& pad = port == 1 ? Ps3Pad::playerSnapshot(1) : Ps3Pad::snapshot();
    out.connected = pad.connected;
    out.leftX = Ps3Input::applyDeadzone(pad.leftX);
    out.leftY = Ps3Input::applyDeadzone(pad.leftY);
    out.rightX = Ps3Input::applyDeadzone(pad.rightX);
    out.rightY = Ps3Input::applyDeadzone(pad.rightY);
    return out;
}

PlatformGamepadSnapshot platformRawGamepadSnapshot(int port)
{
    (void)port;
    PlatformGamepadSnapshot out;
    const Ps3PadSnapshot& pad = Ps3Pad::snapshot();
    out.connected = pad.connected;
    out.leftX = pad.leftX;
    out.leftY = pad.leftY;
    out.rightX = pad.rightX;
    out.rightY = pad.rightY;
    return out;
}

int platformMenuPad()
{
    return Ps3Pad::menuPlayer();
}

// The stick pointer is player 1's; player 2's screens use the D-pad only.
bool platformMenuPointerActive()
{
    return Ps3Pad::menuPlayer() == 0;
}

bool platformMenuCursorVisible()
{
    return Ps3Pad::menuPlayer() == 0;
}

void platformSetMenuCursor(int x, int y)
{
    if (Ps3Pad::menuPlayer() == 0)
        Ps3Input::setMenuCursor(x, y);
}

const PlatformKeyboardHints& platformKeyboardHints()
{
    static const PlatformKeyboardHints hints = {
        {
            "Cross:type  Square:del  Triangle:shift  Select:space  Start:ok  Circle:close",
            "Right stick: move keyboard", nullptr
        }, 2
    };
    return hints;
}

const char* platformInputDebugLine()
{
    return "";
}
