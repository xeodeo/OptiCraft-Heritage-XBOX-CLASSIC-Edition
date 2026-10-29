#pragma once
#ifdef PS3_PLATFORM

// DualShock 3 front end, the PS3 counterpart of src/xbox/input/XboxInput.h:
// reads the pads and maps them onto lwjgl's Mouse/Keyboard event queues the way
// the game expects (menu cursor on the left stick, gameplay bindings).
namespace Ps3Input
{
void poll(bool inMenu, bool specializedMenuNavigation);
}

#endif // PS3_PLATFORM
