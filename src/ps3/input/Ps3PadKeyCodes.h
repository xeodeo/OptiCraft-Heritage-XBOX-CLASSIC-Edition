#pragma once
#ifdef PS3_PLATFORM

#include "lwjgl/Keyboard.h"

// Synthetic key codes for raw DualShock 3 buttons.
//
// Same scheme as src/xbox/input/XboxPadKeyCodes.h and the PS2 one:
// KeyBinding::keyCode is an int shared with the real lwjgl keyboard codes, so
// naming a pad button with a code past KEY_MAX lets the Controls screen,
// rebinding, options.txt persistence and GameSettings::keyName() handle pad
// buttons unchanged.
//
// START (pause) and SELECT (debug overlay) keep fixed roles and are not exposed.
enum Ps3PadKeyCode : int
{
	PS3_KEY_CROSS = lwjgl::Keyboard::KEY_MAX,
	PS3_KEY_CIRCLE,
	PS3_KEY_SQUARE,
	PS3_KEY_TRIANGLE,
	PS3_KEY_R1,
	PS3_KEY_L1,
	PS3_KEY_L2,
	PS3_KEY_R2,
	PS3_KEY_L3,
	PS3_KEY_R3,
	PS3_KEY_DPAD_UP,
	PS3_KEY_DPAD_DOWN,
	PS3_KEY_DPAD_LEFT,
	PS3_KEY_DPAD_RIGHT,
	PS3_KEY_SENTINEL_END
};

static_assert(PS3_KEY_SENTINEL_END < 256, "Ps3PadKeyCode must fit the 256-slot key-state arrays");

// Display name for the Controls screen, or nullptr if `key` isn't one of these.
const char *ps3PadKeyName(int key);

// Key codes the shared Legacy screens ask for by name (see Minecraft.cpp and
// LegacyControlsScreen.cpp): Square is the secondary action, Circle is back.
int ps3PadSquareKeyCode();
int ps3PadCircleKeyCode();

#endif // PS3_PLATFORM
