#ifdef PS3_PLATFORM

#include "ps3/input/Ps3PadKeyCodes.h"

const char *ps3PadKeyName(int key)
{
	switch (key)
	{
		case PS3_KEY_CROSS: return "Cross";
		case PS3_KEY_CIRCLE: return "Circle";
		case PS3_KEY_SQUARE: return "Square";
		case PS3_KEY_TRIANGLE: return "Triangle";
		case PS3_KEY_R1: return "R1";
		case PS3_KEY_L1: return "L1";
		case PS3_KEY_L2: return "L2";
		case PS3_KEY_R2: return "R2";
		case PS3_KEY_L3: return "L3";
		case PS3_KEY_R3: return "R3";
		case PS3_KEY_DPAD_UP: return "D-Pad Up";
		case PS3_KEY_DPAD_DOWN: return "D-Pad Down";
		case PS3_KEY_DPAD_LEFT: return "D-Pad Left";
		case PS3_KEY_DPAD_RIGHT: return "D-Pad Right";
		default: return nullptr;
	}
}

int ps3PadSquareKeyCode()
{
	return PS3_KEY_SQUARE;
}

int ps3PadCircleKeyCode()
{
	return PS3_KEY_CIRCLE;
}

#endif // PS3_PLATFORM
