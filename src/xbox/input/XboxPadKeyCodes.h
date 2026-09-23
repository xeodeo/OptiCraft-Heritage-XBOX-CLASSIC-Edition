#pragma once
#ifdef XBOX_PLATFORM

#include "lwjgl/Keyboard.h"

// Synthetic key codes for raw Xbox controller buttons.
//
// Same scheme as src/ps2/input/Ps2PadKeyCodes.h: KeyBinding::keyCode is an
// int shared with the real lwjgl keyboard codes, so naming a pad button with a
// code past KEY_MAX lets the Controls screen, rebinding, options.txt
// persistence and GameSettings::keyName() handle pad buttons unchanged.
//
// START (pause) and BACK (debug overlay) keep fixed roles and are not exposed.
enum XboxPadKeyCode : int
{
	XBOX_KEY_A = lwjgl::Keyboard::KEY_MAX,
	XBOX_KEY_B,
	XBOX_KEY_X,
	XBOX_KEY_Y,
	XBOX_KEY_BLACK,
	XBOX_KEY_WHITE,
	XBOX_KEY_LEFT_TRIGGER,
	XBOX_KEY_RIGHT_TRIGGER,
	XBOX_KEY_LEFT_THUMB,
	XBOX_KEY_RIGHT_THUMB,
	XBOX_KEY_DPAD_UP,
	XBOX_KEY_DPAD_DOWN,
	XBOX_KEY_DPAD_LEFT,
	XBOX_KEY_DPAD_RIGHT,
	XBOX_KEY_SENTINEL_END
};

static_assert(XBOX_KEY_SENTINEL_END < 256, "XboxPadKeyCode must fit the 256-slot key-state arrays");

// Display name for the Controls screen, or nullptr if `key` isn't one of these.
const char *xboxPadKeyName(int key);

#endif // XBOX_PLATFORM
