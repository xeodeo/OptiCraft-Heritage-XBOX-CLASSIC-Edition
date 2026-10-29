// Ps3Input.cpp — maps the DualShock 3 controller onto lwjgl keyboard/mouse events.
//
// A direct port of src/xbox/input/XboxInput.cpp: same menu cursor, analog menu
// navigation, rebind capture and gameplay synthesis, with the Xbox buttons
// swapped for their DualShock 3 positions:
//
//   Xbox A/B/X/Y           -> Cross/Circle/Square/Triangle
//   Xbox White/Black       -> L1/R1 (hotbar scroll / tab switch)
//   Xbox LT/RT             -> L2/R2 (use/attack)
//   Xbox left/right thumb  -> L3/R3
//   Xbox Back/Start        -> Select/Start
#ifdef PS3_PLATFORM

#include "ps3/input/Ps3Input.h"
#include "ps3/input/Ps3Pad.h"
#include "ps3/input/Ps3PadKeyCodes.h"
#include "ps3/system/Ps3Video.h"

struct SDL_Event;
#include "lwjgl/Keyboard.h"
#include "lwjgl/Mouse.h"
#include "platform/Input.h"
#include "platform/time.h"

namespace lwjgl
{
namespace Keyboard { namespace detail { void pushKey(int lwjglKey, bool down); } }
namespace Mouse
{
namespace detail
{
void pushMotion(int x, int y, int xrel, int yrel);
void pushButton(int button, bool down, int x, int y);
void pushWheel(int delta, int x, int y);
}
}
}

namespace
{
// Back-buffer size; read when used, not during static initialisation.
#define kScreenWidth (Ps3Video::width())
#define kScreenHeight (Ps3Video::height())

const float MENU_SCROLL_THRESHOLD = 0.18f;
const float CAM_SCALE = 14.0f;
const int CAMERA_WARMUP_FRAMES = 18;
const float MENU_NAV_ENTER_THRESHOLD = 0.60f;
const float MENU_NAV_RELEASE_THRESHOLD = 0.35f;
const float MENU_NAV_REPEAT_DELAY = 0.30f;
const float MENU_NAV_REPEAT_INTERVAL = 0.11f;

float s_deadzone = 0.20f;
bool s_gameplayKeyDown[256] = {};
bool s_previousMenu = false;
// Buttons still held when a menu closes (Circle closes the inventory and is also
// "drop item" in game): ignored by gameplay until released.
unsigned int s_suppressedButtons = 0;
int s_cameraWarmup = 0;
// Select + D-pad in game: Up saves a waypoint, Down opens the waypoint manager.
// Select alone still toggles the debug screen, on release, so a combo does not
// also open it.
bool s_selectCombo = false;
int s_waypointRequest = 0;
int s_menuAnalogDirection = 0;
float s_menuAnalogRepeat = 0.0f;

// --- menu pointer (Ps2Pointer) -----------------------------------------------
float s_pointerX = 320.0f;   // recentred by pointerEnterMenu()
float s_pointerY = 240.0f;
float s_pointerLastTime = 0.0f;
int s_publishedX = -1;
int s_publishedY = -1;

void clampPointer()
{
	if (s_pointerX < 0.0f) s_pointerX = 0.0f;
	if (s_pointerY < 0.0f) s_pointerY = 0.0f;
	if (s_pointerX > kScreenWidth - 1) s_pointerX = static_cast<float>(kScreenWidth - 1);
	if (s_pointerY > kScreenHeight - 1) s_pointerY = static_cast<float>(kScreenHeight - 1);
}

void pointerEnterMenu()
{
	s_pointerX = kScreenWidth * 0.5f;
	s_pointerY = kScreenHeight * 0.5f;
	s_pointerLastTime = getTimeS();
	s_publishedX = -1;
	s_publishedY = -1;
}

void pointerLeaveMenu()
{
	s_pointerLastTime = 0.0f;
	s_publishedX = -1;
	s_publishedY = -1;
}

float pointerBeginFrame()
{
	const float now = getTimeS();
	float dt = (s_pointerLastTime > 0.0f) ? now - s_pointerLastTime : 0.0f;
	s_pointerLastTime = now;
	if (dt < 0.0f) dt = 0.0f;
	if (dt > 0.10f) dt = 0.10f;
	return dt;
}

void pointerPublish()
{
	const int x = static_cast<int>(s_pointerX);
	const int y = static_cast<int>(s_pointerY);
	if (x == s_publishedX && y == s_publishedY)
		return;
	const int dx = s_publishedX < 0 ? 0 : x - s_publishedX;
	const int dy = s_publishedY < 0 ? 0 : y - s_publishedY;
	s_publishedX = x;
	s_publishedY = y;
	lwjgl::Mouse::detail::pushMotion(x, y, dx, dy);
}

// --- helpers -----------------------------------------------------------------
float deadzone(float v, float dz) { return (v > -dz && v < dz) ? 0.0f : v; }
float absoluteValue(float v) { return v < 0.0f ? -v : v; }

void setKey(int key, bool down)
{
	if (key < 0 || key >= 256 || s_gameplayKeyDown[key] == down)
		return;
	s_gameplayKeyDown[key] = down;
	lwjgl::Keyboard::detail::pushKey(key, down);
}

void releaseGameplayKeys()
{
	for (int key = PS3_KEY_CROSS; key < PS3_KEY_SENTINEL_END; ++key)
		setKey(key, false);
}

unsigned short menuAnalogDirectionMask(int direction)
{
	switch (direction)
	{
		case 1: return PS3_PAD_DPAD_UP;
		case 2: return PS3_PAD_DPAD_DOWN;
		case 3: return PS3_PAD_DPAD_LEFT;
		case 4: return PS3_PAD_DPAD_RIGHT;
		default: return 0;
	}
}

int menuAnalogDirection(float x, float y)
{
	const float absX = absoluteValue(x);
	const float absY = absoluteValue(y);
	if (absX < MENU_NAV_ENTER_THRESHOLD && absY < MENU_NAV_ENTER_THRESHOLD)
		return 0;
	if (absY >= absX)
		return y < 0.0f ? 1 : 2;
	return x < 0.0f ? 3 : 4;
}

unsigned short updateMenuAnalogNavigation(const Ps3PadSnapshot& pad, float dt, bool active)
{
	if (!active)
	{
		s_menuAnalogDirection = 0;
		s_menuAnalogRepeat = 0.0f;
		return 0;
	}

	const float x = Ps3Input::applyDeadzone(pad.leftX);
	const float y = Ps3Input::applyDeadzone(pad.leftY);

	if (s_menuAnalogDirection != 0)
	{
		const bool vertical = s_menuAnalogDirection <= 2;
		const float activeAxis = vertical ? absoluteValue(y) : absoluteValue(x);
		if (activeAxis <= MENU_NAV_RELEASE_THRESHOLD)
		{
			s_menuAnalogDirection = 0;
			s_menuAnalogRepeat = 0.0f;
		}
	}

	if (s_menuAnalogDirection == 0)
	{
		const int direction = menuAnalogDirection(x, y);
		if (direction == 0)
			return 0;
		s_menuAnalogDirection = direction;
		s_menuAnalogRepeat = MENU_NAV_REPEAT_DELAY;
		return menuAnalogDirectionMask(direction);
	}

	s_menuAnalogRepeat -= dt;
	if (s_menuAnalogRepeat > 0.0f)
		return 0;

	s_menuAnalogRepeat = MENU_NAV_REPEAT_INTERVAL;
	return menuAnalogDirectionMask(s_menuAnalogDirection);
}

// Buttons a Controls-menu binding can learn (Start/Select stay reserved).
const struct { unsigned short mask; int code; } kRemappableButtons[] = {
	{ PS3_PAD_CROSS, PS3_KEY_CROSS },               { PS3_PAD_CIRCLE, PS3_KEY_CIRCLE },
	{ PS3_PAD_SQUARE, PS3_KEY_SQUARE },             { PS3_PAD_TRIANGLE, PS3_KEY_TRIANGLE },
	{ PS3_PAD_R1, PS3_KEY_R1 },                     { PS3_PAD_L1, PS3_KEY_L1 },
	{ PS3_PAD_L2, PS3_KEY_L2 },                     { PS3_PAD_R2, PS3_KEY_R2 },
	{ PS3_PAD_L3, PS3_KEY_L3 },                     { PS3_PAD_R3, PS3_KEY_R3 },
	{ PS3_PAD_DPAD_UP, PS3_KEY_DPAD_UP },           { PS3_PAD_DPAD_DOWN, PS3_KEY_DPAD_DOWN },
	{ PS3_PAD_DPAD_LEFT, PS3_KEY_DPAD_LEFT },       { PS3_PAD_DPAD_RIGHT, PS3_KEY_DPAD_RIGHT },
};

void pushKeyEdge(const Ps3PadSnapshot& p, unsigned short mask, int key)
{
	if (p.pressed & mask) lwjgl::Keyboard::detail::pushKey(key, true);
	if (p.released & mask) lwjgl::Keyboard::detail::pushKey(key, false);
}

void updateMenu(const Ps3PadSnapshot& p, bool specializedMenuNavigation)
{
	if (platformTextInputExclusive())
		return;

	if (platformPadRebindExclusive())
	{
		// GuiControls is waiting for a new binding: report one pressed button.
		for (const auto& entry : kRemappableButtons)
		{
			if (p.pressed & entry.mask)
			{
				lwjgl::Keyboard::detail::pushKey(entry.code, true);
				lwjgl::Keyboard::detail::pushKey(entry.code, false);
				break;
			}
		}
		return;
	}

	const float dt = pointerBeginFrame();
	const float dpadSpeed = 300.0f, analogSpeed = 420.0f;
	const bool rightThumb = (p.held & PS3_PAD_R3) != 0;
	const bool slotNav = platformContainerNavigationActive();

	if (specializedMenuNavigation)
	{
		const unsigned short analogPressed = updateMenuAnalogNavigation(p, dt, true);
		if (analogPressed != 0)
			Ps3Pad::latchPressed(analogPressed);
	}
	else
	{
		updateMenuAnalogNavigation(p, dt, false);
		float dx = 0.0f, dy = 0.0f;
		if (!rightThumb && !slotNav && (p.held & PS3_PAD_DPAD_UP)) dy -= dpadSpeed * dt;
		if (!rightThumb && !slotNav && (p.held & PS3_PAD_DPAD_DOWN)) dy += dpadSpeed * dt;
		if (!slotNav && (p.held & PS3_PAD_DPAD_LEFT)) dx -= dpadSpeed * dt;
		if (!slotNav && (p.held & PS3_PAD_DPAD_RIGHT)) dx += dpadSpeed * dt;
		dx += Ps3Input::applyDeadzone(p.leftX) * analogSpeed * dt;
		dy += Ps3Input::applyDeadzone(p.leftY) * analogSpeed * dt;
		s_pointerX += dx;
		s_pointerY += dy;
		clampPointer();
		pointerPublish();
	}
	const int cx = static_cast<int>(s_pointerX), cy = static_cast<int>(s_pointerY);

	static float scrollRepeat = 0.0f;
	if (rightThumb && (p.held & (PS3_PAD_DPAD_UP | PS3_PAD_DPAD_DOWN)))
	{
		bool fire = (p.pressed & (PS3_PAD_DPAD_UP | PS3_PAD_DPAD_DOWN)) != 0;
		scrollRepeat -= dt;
		if (scrollRepeat <= 0.0f) { fire = true; scrollRepeat = 0.12f; }
		if (fire) lwjgl::Mouse::detail::pushWheel((p.held & PS3_PAD_DPAD_UP) ? 1 : -1, cx, cy);
	}
	else
	{
		scrollRepeat = 0.0f;
	}

	static float stickScroll = 0.0f;
	const float rsv = deadzone(Ps3Input::applyDeadzone(p.rightY), MENU_SCROLL_THRESHOLD);
	if (rsv != 0.0f)
	{
		const float mag = rsv < 0.0f ? -rsv : rsv;
		const float interval = 0.20f - 0.16f * mag;
		stickScroll -= dt;
		if (stickScroll <= 0.0f)
		{
			stickScroll = interval;
			lwjgl::Mouse::detail::pushWheel(rsv < 0.0f ? 1 : -1, cx, cy);
		}
	}
	else
	{
		stickScroll = 0.0f;
	}

	if (!specializedMenuNavigation)
	{
		if (p.pressed & PS3_PAD_CROSS) lwjgl::Mouse::detail::pushButton(0, true, cx, cy);
		if (p.released & PS3_PAD_CROSS) lwjgl::Mouse::detail::pushButton(0, false, cx, cy);
		if (p.pressed & PS3_PAD_SQUARE) lwjgl::Mouse::detail::pushButton(1, true, cx, cy);
		if (p.released & PS3_PAD_SQUARE) lwjgl::Mouse::detail::pushButton(1, false, cx, cy);
		pushKeyEdge(p, PS3_PAD_CIRCLE | PS3_PAD_TRIANGLE, lwjgl::Keyboard::KEY_ESCAPE);
		pushKeyEdge(p, PS3_PAD_START, lwjgl::Keyboard::KEY_RETURN);
	}
	pushKeyEdge(p, PS3_PAD_SELECT, lwjgl::Keyboard::KEY_TAB);
}

void updateGameplay(const Ps3PadSnapshot& p)
{
	if (s_cameraWarmup > 0)
	{
		--s_cameraWarmup;
		lwjgl::Mouse::clearDeltas();
	}
	else
	{
		const float camX = Ps3Input::applyDeadzone(p.rightX);
		const float camY = Ps3Input::applyDeadzone(p.rightY);
		int dx = static_cast<int>(camX * CAM_SCALE), dy = static_cast<int>(camY * CAM_SCALE);
		if (dx > -2 && dx < 2) dx = 0;
		if (dy > -2 && dy < 2) dy = 0;
		if (dx || dy) lwjgl::Mouse::detail::pushMotion(0, 0, dx, dy);
	}

	// Every remappable button gets its own synthetic key; GameSettings decides
	// which action each drives (defaults in GameSettingsBackend_PS3.cpp).
	const float moveX = Ps3Input::applyDeadzone(p.leftX);
	const float moveY = Ps3Input::applyDeadzone(p.leftY);
	const bool selectHeld = (p.held & PS3_PAD_SELECT) != 0;
	if (p.pressed & PS3_PAD_SELECT)
		s_selectCombo = false;
	if (selectHeld && (p.pressed & PS3_PAD_DPAD_UP))
	{
		s_waypointRequest = 1;
		s_selectCombo = true;
	}
	else if (selectHeld && (p.pressed & PS3_PAD_DPAD_DOWN))
	{
		s_waypointRequest = 2;
		s_selectCombo = true;
	}
	// The D-pad does not walk while it is part of a Select combo.
	const unsigned short dpadHeld = selectHeld ? 0 : p.held;
	setKey(PS3_KEY_DPAD_UP, moveY < -0.05f || (dpadHeld & PS3_PAD_DPAD_UP));
	setKey(PS3_KEY_DPAD_DOWN, moveY > 0.05f || (dpadHeld & PS3_PAD_DPAD_DOWN));
	setKey(PS3_KEY_DPAD_LEFT, moveX < -0.05f || (dpadHeld & PS3_PAD_DPAD_LEFT));
	setKey(PS3_KEY_DPAD_RIGHT, moveX > 0.05f || (dpadHeld & PS3_PAD_DPAD_RIGHT));
	setKey(PS3_KEY_CROSS, (p.held & PS3_PAD_CROSS) != 0);
	setKey(PS3_KEY_CIRCLE, (p.held & PS3_PAD_CIRCLE) != 0);
	setKey(PS3_KEY_SQUARE, (p.held & PS3_PAD_SQUARE) != 0);
	setKey(PS3_KEY_TRIANGLE, (p.held & PS3_PAD_TRIANGLE) != 0);
	setKey(PS3_KEY_R1, (p.held & PS3_PAD_R1) != 0);
	setKey(PS3_KEY_L1, (p.held & PS3_PAD_L1) != 0);
	setKey(PS3_KEY_L2, (p.held & PS3_PAD_L2) != 0);
	setKey(PS3_KEY_R2, (p.held & PS3_PAD_R2) != 0);
	setKey(PS3_KEY_L3, (p.held & PS3_PAD_L3) != 0);
	setKey(PS3_KEY_R3, (p.held & PS3_PAD_R3) != 0);

	if (p.pressed & PS3_PAD_R2) lwjgl::Mouse::detail::pushButton(0, true, 0, 0);
	if (p.released & PS3_PAD_R2) lwjgl::Mouse::detail::pushButton(0, false, 0, 0);
	if (p.pressed & PS3_PAD_L2) lwjgl::Mouse::detail::pushButton(1, true, 0, 0);
	if (p.released & PS3_PAD_L2) lwjgl::Mouse::detail::pushButton(1, false, 0, 0);
	pushKeyEdge(p, PS3_PAD_START, lwjgl::Keyboard::KEY_ESCAPE);
	if (p.pressed & PS3_PAD_R1) lwjgl::Mouse::detail::pushWheel(-1, 0, 0);
	if (p.pressed & PS3_PAD_L1) lwjgl::Mouse::detail::pushWheel(1, 0, 0);
	pushKeyEdge(p, PS3_PAD_R3, lwjgl::Keyboard::KEY_F5);
	if (p.released & PS3_PAD_SELECT)
	{
		if (!s_selectCombo)
		{
			lwjgl::Keyboard::detail::pushKey(lwjgl::Keyboard::KEY_F3, true);
			lwjgl::Keyboard::detail::pushKey(lwjgl::Keyboard::KEY_F3, false);
		}
		s_selectCombo = false;
	}

	// Gameplay does not use the text/menu latch; keep it from leaking into menus.
	Ps3Pad::clearLatchedPressed();
}
} // namespace

namespace Ps3Input
{
int consumeWaypointRequest()
{
	const int request = s_waypointRequest;
	s_waypointRequest = 0;
	return request;
}

void initialize()
{
	Ps3Pad::initialize();
}

float applyDeadzone(float value)
{
	if (value > -s_deadzone && value < s_deadzone)
		return 0.0f;
	const float sign = value < 0.0f ? -1.0f : 1.0f;
	float output = (value * sign - s_deadzone) / (1.0f - s_deadzone);
	if (output < 0.0f) output = 0.0f;
	if (output > 1.0f) output = 1.0f;
	return output * sign;
}

void poll(bool inMenu, bool specializedMenuNavigation)
{
	Ps3Pad::poll();
	const Ps3PadSnapshot& pad = Ps3Pad::snapshot();

	if (inMenu && !s_previousMenu)
	{
		releaseGameplayKeys();
		Ps3Pad::clearLatchedPressed();
		pointerEnterMenu();
	}
	if (!inMenu && s_previousMenu)
	{
		s_suppressedButtons = pad.held;
		s_cameraWarmup = CAMERA_WARMUP_FRAMES;
		lwjgl::Mouse::clearDeltas();
		Ps3Pad::clearLatchedPressed();
		pointerLeaveMenu();
	}
	s_previousMenu = inMenu;

	if (!pad.connected)
	{
		if (!inMenu)
			releaseGameplayKeys();
		return;
	}
	if (inMenu)
	{
		s_suppressedButtons = 0;
		updateMenu(pad, specializedMenuNavigation);
	}
	else
	{
		s_suppressedButtons &= pad.held;   // a released button counts again
		if (s_suppressedButtons == 0)
		{
			updateGameplay(pad);
		}
		else
		{
			Ps3PadSnapshot filtered = pad;
			filtered.held &= ~s_suppressedButtons;
			filtered.pressed &= ~s_suppressedButtons;
			filtered.released &= ~s_suppressedButtons;
			updateGameplay(filtered);
		}
	}
}

void setMenuCursor(int x, int y)
{
	s_pointerX = static_cast<float>(x);
	s_pointerY = static_cast<float>(y);
	clampPointer();
	pointerPublish();
}

} // namespace Ps3Input

#endif // PS3_PLATFORM
