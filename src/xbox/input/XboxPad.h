#pragma once
#ifdef XBOX_PLATFORM

// Normalized controller state, same shape as src/ps2/input/Ps2PadState.h so
// the mapper can follow the PS2 one closely. Sticks are -1..1 with Y down
// positive (up is negative, as on PS2); analog buttons become digital bits.
enum XboxPadButton : unsigned short
{
	XBOX_PAD_DPAD_UP     = 0x0001,
	XBOX_PAD_DPAD_DOWN   = 0x0002,
	XBOX_PAD_DPAD_LEFT   = 0x0004,
	XBOX_PAD_DPAD_RIGHT  = 0x0008,
	XBOX_PAD_START       = 0x0010,
	XBOX_PAD_BACK        = 0x0020,
	XBOX_PAD_LEFT_THUMB  = 0x0040,
	XBOX_PAD_RIGHT_THUMB = 0x0080,
	XBOX_PAD_A           = 0x0100,
	XBOX_PAD_B           = 0x0200,
	XBOX_PAD_X           = 0x0400,
	XBOX_PAD_Y           = 0x0800,
	XBOX_PAD_BLACK       = 0x1000,
	XBOX_PAD_WHITE       = 0x2000,
	XBOX_PAD_LT          = 0x4000,
	XBOX_PAD_RT          = 0x8000
};

struct XboxPadSnapshot
{
	bool connected;
	float leftX;
	float leftY;
	float rightX;
	float rightY;
	unsigned short held;
	unsigned short pressed;
	unsigned short released;
};

namespace XboxPad
{
// XInitDevices + open whatever is plugged in. Safe to call more than once.
void initialize();
// Reads the controller once per frame (inserts/removals handled here).
void poll();
// Player 1 = the first connected controller, whatever port it is in.
const XboxPadSnapshot& snapshot();
// Buttons pressed since the last consume (text entry reads these).
unsigned short consumePressed();
void clearLatchedPressed();
void latchPressed(unsigned short pressed);
}

#endif // XBOX_PLATFORM
