#ifdef XBOX_PLATFORM

#include "xbox/input/XboxPadKeyCodes.h"

const char *xboxPadKeyName(int key)
{
	switch (key)
	{
		case XBOX_KEY_A: return "A";
		case XBOX_KEY_B: return "B";
		case XBOX_KEY_X: return "X";
		case XBOX_KEY_Y: return "Y";
		case XBOX_KEY_BLACK: return "Black";
		case XBOX_KEY_WHITE: return "White";
		case XBOX_KEY_LEFT_TRIGGER: return "LT";
		case XBOX_KEY_RIGHT_TRIGGER: return "RT";
		case XBOX_KEY_LEFT_THUMB: return "Left Stick";
		case XBOX_KEY_RIGHT_THUMB: return "Right Stick";
		case XBOX_KEY_DPAD_UP: return "D-Pad Up";
		case XBOX_KEY_DPAD_DOWN: return "D-Pad Down";
		case XBOX_KEY_DPAD_LEFT: return "D-Pad Left";
		case XBOX_KEY_DPAD_RIGHT: return "D-Pad Right";
		default: return nullptr;
	}
}

#endif // XBOX_PLATFORM
