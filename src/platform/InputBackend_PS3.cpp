// PS3 implementation of platform/Input.h.
//
// Placeholder until src/ps3/input (the DualShock 3 port of src/xbox/input) lands:
// reports no controller, so the menu renders but cannot be driven yet.
#include "platform/Input.h"

PlatformTextInputSnapshot platformTextInputSnapshot(int)
{
    return PlatformTextInputSnapshot();
}

PlatformGamepadSnapshot platformGamepadSnapshot(int)
{
    return PlatformGamepadSnapshot();
}

PlatformGamepadSnapshot platformRawGamepadSnapshot(int)
{
    return PlatformGamepadSnapshot();
}

int platformMenuPad() { return 0; }
bool platformMenuPointerActive() { return true; }
bool platformMenuCursorVisible() { return true; }
void platformSetMenuCursor(int, int) {}

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
