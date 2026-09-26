// Xbox implementation of platform/Input.h, over src/xbox/input (same shape as
// InputBackend_PS2.cpp: one controller, stick-driven menu cursor).
#include "platform/Input.h"
#include "platform/Log.h"

#include "xbox/input/XboxInput.h"
#include "xbox/input/XboxPad.h"

namespace
{
std::uint32_t mapTextButtons(unsigned short bits)
{
    std::uint32_t value = 0;
    if (bits & XBOX_PAD_DPAD_LEFT) value |= PLATFORM_TEXT_LEFT;
    if (bits & XBOX_PAD_DPAD_RIGHT) value |= PLATFORM_TEXT_RIGHT;
    if (bits & XBOX_PAD_DPAD_UP) value |= PLATFORM_TEXT_UP;
    if (bits & XBOX_PAD_DPAD_DOWN) value |= PLATFORM_TEXT_DOWN;
    if (bits & XBOX_PAD_A) value |= PLATFORM_TEXT_TYPE;
    if (bits & XBOX_PAD_X) value |= PLATFORM_TEXT_BACK;
    if (bits & XBOX_PAD_BACK) value |= PLATFORM_TEXT_SPACE;
    if (bits & XBOX_PAD_Y) value |= PLATFORM_TEXT_SHIFT;
    if (bits & XBOX_PAD_START) value |= PLATFORM_TEXT_ENTER;
    if (bits & XBOX_PAD_B) value |= PLATFORM_TEXT_CLOSE;
    if (bits & XBOX_PAD_WHITE) value |= PLATFORM_TEXT_TAB_LEFT;
    if (bits & XBOX_PAD_BLACK) value |= PLATFORM_TEXT_TAB_RIGHT;
    return value;
}
}

PlatformTextInputSnapshot platformTextInputSnapshot(int port)
{
    (void)port;
    PlatformTextInputSnapshot out;
    const XboxPadSnapshot& pad = XboxPad::snapshot();
    out.connected = pad.connected;
    out.held = mapTextButtons(pad.held);
    out.pressed = mapTextButtons(XboxPad::consumePressed());
#if MC_LOG_LEVEL > 0
    if (out.pressed != 0)
        MC_LOG_INFO("xbox.input", "text pressed=%04x held=%04x\n", out.pressed, out.held);
#endif
    return out;
}

PlatformGamepadSnapshot platformGamepadSnapshot(int port)
{
    (void)port;
    PlatformGamepadSnapshot out;
    const XboxPadSnapshot& pad = XboxPad::snapshot();
    out.connected = pad.connected;
    out.leftX = XboxInput::applyDeadzone(pad.leftX);
    out.leftY = XboxInput::applyDeadzone(pad.leftY);
    out.rightX = XboxInput::applyDeadzone(pad.rightX);
    out.rightY = XboxInput::applyDeadzone(pad.rightY);
    return out;
}

PlatformGamepadSnapshot platformRawGamepadSnapshot(int port)
{
    (void)port;
    PlatformGamepadSnapshot out;
    const XboxPadSnapshot& pad = XboxPad::snapshot();
    out.connected = pad.connected;
    out.leftX = pad.leftX;
    out.leftY = pad.leftY;
    out.rightX = pad.rightX;
    out.rightY = pad.rightY;
    return out;
}

int platformMenuPad()
{
    return 0;
}

bool platformMenuPointerActive()
{
    return true;
}

bool platformMenuCursorVisible()
{
    return true;
}

void platformSetMenuCursor(int x, int y)
{
    XboxInput::setMenuCursor(x, y);
}

const PlatformKeyboardHints& platformKeyboardHints()
{
    static const PlatformKeyboardHints hints = {
        {
            "A:type  X:del  Y:shift  Back:space  Start:ok  B:close",
            "Right stick: move keyboard", nullptr
        }, 2
    };
    return hints;
}

const char* platformInputDebugLine()
{
    return "";
}
