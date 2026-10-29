#pragma once
#ifdef PS3_PLATFORM

// Normalized controller state, same shape as src/xbox/input/XboxPad.h and
// src/ps2/input/Ps2PadState.h so the mapper can follow XboxInput closely.
// Sticks are -1..1 with Y down positive (up is negative, as on Xbox/PS2).
enum Ps3PadButton : unsigned short
{
	PS3_PAD_DPAD_UP     = 0x0001,
	PS3_PAD_DPAD_DOWN   = 0x0002,
	PS3_PAD_DPAD_LEFT   = 0x0004,
	PS3_PAD_DPAD_RIGHT  = 0x0008,
	PS3_PAD_START       = 0x0010,
	PS3_PAD_SELECT      = 0x0020,
	PS3_PAD_L3          = 0x0040,
	PS3_PAD_R3          = 0x0080,
	PS3_PAD_CROSS       = 0x0100,
	PS3_PAD_CIRCLE      = 0x0200,
	PS3_PAD_SQUARE      = 0x0400,
	PS3_PAD_TRIANGLE    = 0x0800,
	PS3_PAD_R1          = 0x1000,
	PS3_PAD_L1          = 0x2000,
	PS3_PAD_L2          = 0x4000,
	PS3_PAD_R2          = 0x8000
};

struct Ps3PadSnapshot
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

namespace Ps3Pad
{
// ioPadInit(7). Safe to call more than once.
void initialize();
// Reads the controllers once per frame (inserts/removals handled here).
void poll();
// Player 1 = the first connected controller, whatever port it is in.
const Ps3PadSnapshot& snapshot();
// Raw per-player state: 0 = first connected controller, 1 = the second.
// setMenuPlayer picks whose presses consumePressed/clear/latchPressed use
// (1 only while Minecraft runs player 2's screen, see enterPlayer2Context).
const Ps3PadSnapshot& playerSnapshot(int player);
void setMenuPlayer(int player);
int menuPlayer();
// Buttons pressed since the last consume (text entry reads these).
unsigned short consumePressed();
void clearLatchedPressed();
void clearLatchedPressed(int player);
void latchPressed(unsigned short pressed);
// L1/R1 presses since the last call, whoever consumed the menu presses
// (tab switching read on a screen's tick).
unsigned short consumePagePressed();
}

#endif // PS3_PLATFORM
