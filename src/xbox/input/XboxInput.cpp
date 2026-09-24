// XboxInput.cpp — maps the Xbox controller onto lwjgl keyboard/mouse events.
//
// A port of src/ps2/input/Ps2InputMapper.cpp (+ Ps2Pointer, Ps2AnalogFilter):
// same menu cursor, analog menu navigation, rebind capture and gameplay
// synthesis, with the PS2 buttons swapped for their Xbox positions:
//
//   PS2 Cross/Circle/Square/Triangle -> A/B/X/Y
//   PS2 L1/R1 (hotbar scroll)        -> White/Black
//   PS2 L2/R2 (use/attack)           -> LT/RT
//   PS2 L3/R3                        -> left/right stick click
//   PS2 Select/Start                 -> Back/Start
#ifdef XBOX_PLATFORM

#include "xbox/input/XboxInput.h"
#include "xbox/input/XboxPad.h"
#include "xbox/input/XboxPadKeyCodes.h"
#include "xbox/system/XboxVideoMode.h"

#include "lwjgl/Keyboard.h"
#include "lwjgl/Mouse.h"
#include "platform/Input.h"
#include "platform/time.h"

namespace
{
// Back-buffer size (640x480, or 1280x720 for the 720p XBE); read when used,
// not during static initialisation, which runs before the log exists.
#define kScreenWidth (XboxVideoMode::width())
#define kScreenHeight (XboxVideoMode::height())

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
// Buttons still held when a menu closes (B closes the inventory and is also
// "drop item" in game): ignored by gameplay until released.
unsigned int s_suppressedButtons = 0;
int s_cameraWarmup = 0;
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
	for (int key = XBOX_KEY_A; key < XBOX_KEY_SENTINEL_END; ++key)
		setKey(key, false);
}

unsigned short menuAnalogDirectionMask(int direction)
{
	switch (direction)
	{
		case 1: return XBOX_PAD_DPAD_UP;
		case 2: return XBOX_PAD_DPAD_DOWN;
		case 3: return XBOX_PAD_DPAD_LEFT;
		case 4: return XBOX_PAD_DPAD_RIGHT;
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

unsigned short updateMenuAnalogNavigation(const XboxPadSnapshot& pad, float dt, bool active)
{
	if (!active)
	{
		s_menuAnalogDirection = 0;
		s_menuAnalogRepeat = 0.0f;
		return 0;
	}

	const float x = XboxInput::applyDeadzone(pad.leftX);
	const float y = XboxInput::applyDeadzone(pad.leftY);

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

// Buttons a Controls-menu binding can learn (Start/Back stay reserved).
const struct { unsigned short mask; int code; } kRemappableButtons[] = {
	{ XBOX_PAD_A, XBOX_KEY_A },                     { XBOX_PAD_B, XBOX_KEY_B },
	{ XBOX_PAD_X, XBOX_KEY_X },                     { XBOX_PAD_Y, XBOX_KEY_Y },
	{ XBOX_PAD_BLACK, XBOX_KEY_BLACK },             { XBOX_PAD_WHITE, XBOX_KEY_WHITE },
	{ XBOX_PAD_LT, XBOX_KEY_LEFT_TRIGGER },         { XBOX_PAD_RT, XBOX_KEY_RIGHT_TRIGGER },
	{ XBOX_PAD_LEFT_THUMB, XBOX_KEY_LEFT_THUMB },   { XBOX_PAD_RIGHT_THUMB, XBOX_KEY_RIGHT_THUMB },
	{ XBOX_PAD_DPAD_UP, XBOX_KEY_DPAD_UP },         { XBOX_PAD_DPAD_DOWN, XBOX_KEY_DPAD_DOWN },
	{ XBOX_PAD_DPAD_LEFT, XBOX_KEY_DPAD_LEFT },     { XBOX_PAD_DPAD_RIGHT, XBOX_KEY_DPAD_RIGHT },
};

void pushKeyEdge(const XboxPadSnapshot& p, unsigned short mask, int key)
{
	if (p.pressed & mask) lwjgl::Keyboard::detail::pushKey(key, true);
	if (p.released & mask) lwjgl::Keyboard::detail::pushKey(key, false);
}

void updateMenu(const XboxPadSnapshot& p, bool specializedMenuNavigation)
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
	const bool rightThumb = (p.held & XBOX_PAD_RIGHT_THUMB) != 0;
	const bool slotNav = platformContainerNavigationActive();

	if (specializedMenuNavigation)
	{
		const unsigned short analogPressed = updateMenuAnalogNavigation(p, dt, true);
		if (analogPressed != 0)
			XboxPad::latchPressed(analogPressed);
	}
	else
	{
		updateMenuAnalogNavigation(p, dt, false);
		float dx = 0.0f, dy = 0.0f;
		if (!rightThumb && !slotNav && (p.held & XBOX_PAD_DPAD_UP)) dy -= dpadSpeed * dt;
		if (!rightThumb && !slotNav && (p.held & XBOX_PAD_DPAD_DOWN)) dy += dpadSpeed * dt;
		if (!slotNav && (p.held & XBOX_PAD_DPAD_LEFT)) dx -= dpadSpeed * dt;
		if (!slotNav && (p.held & XBOX_PAD_DPAD_RIGHT)) dx += dpadSpeed * dt;
		dx += XboxInput::applyDeadzone(p.leftX) * analogSpeed * dt;
		dy += XboxInput::applyDeadzone(p.leftY) * analogSpeed * dt;
		s_pointerX += dx;
		s_pointerY += dy;
		clampPointer();
		pointerPublish();
	}
	const int cx = static_cast<int>(s_pointerX), cy = static_cast<int>(s_pointerY);

	static float scrollRepeat = 0.0f;
	if (rightThumb && (p.held & (XBOX_PAD_DPAD_UP | XBOX_PAD_DPAD_DOWN)))
	{
		bool fire = (p.pressed & (XBOX_PAD_DPAD_UP | XBOX_PAD_DPAD_DOWN)) != 0;
		scrollRepeat -= dt;
		if (scrollRepeat <= 0.0f) { fire = true; scrollRepeat = 0.12f; }
		if (fire) lwjgl::Mouse::detail::pushWheel((p.held & XBOX_PAD_DPAD_UP) ? 1 : -1, cx, cy);
	}
	else
	{
		scrollRepeat = 0.0f;
	}

	static float stickScroll = 0.0f;
	const float rsv = deadzone(XboxInput::applyDeadzone(p.rightY), MENU_SCROLL_THRESHOLD);
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
		if (p.pressed & XBOX_PAD_A) lwjgl::Mouse::detail::pushButton(0, true, cx, cy);
		if (p.released & XBOX_PAD_A) lwjgl::Mouse::detail::pushButton(0, false, cx, cy);
		if (p.pressed & XBOX_PAD_X) lwjgl::Mouse::detail::pushButton(1, true, cx, cy);
		if (p.released & XBOX_PAD_X) lwjgl::Mouse::detail::pushButton(1, false, cx, cy);
		pushKeyEdge(p, XBOX_PAD_B | XBOX_PAD_Y, lwjgl::Keyboard::KEY_ESCAPE);
		pushKeyEdge(p, XBOX_PAD_START, lwjgl::Keyboard::KEY_RETURN);
	}
	pushKeyEdge(p, XBOX_PAD_BACK, lwjgl::Keyboard::KEY_TAB);
}

void updateGameplay(const XboxPadSnapshot& p)
{
	if (s_cameraWarmup > 0)
	{
		--s_cameraWarmup;
		lwjgl::Mouse::clearDeltas();
	}
	else
	{
		const float camX = XboxInput::applyDeadzone(p.rightX);
		const float camY = XboxInput::applyDeadzone(p.rightY);
		int dx = static_cast<int>(camX * CAM_SCALE), dy = static_cast<int>(camY * CAM_SCALE);
		if (dx > -2 && dx < 2) dx = 0;
		if (dy > -2 && dy < 2) dy = 0;
		if (dx || dy) lwjgl::Mouse::detail::pushMotion(0, 0, dx, dy);
	}

	// Every remappable button gets its own synthetic key; GameSettings decides
	// which action each drives (defaults in GameSettingsBackend_XBOX.cpp).
	const float moveX = XboxInput::applyDeadzone(p.leftX);
	const float moveY = XboxInput::applyDeadzone(p.leftY);
	setKey(XBOX_KEY_DPAD_UP, moveY < -0.05f || (p.held & XBOX_PAD_DPAD_UP));
	setKey(XBOX_KEY_DPAD_DOWN, moveY > 0.05f || (p.held & XBOX_PAD_DPAD_DOWN));
	setKey(XBOX_KEY_DPAD_LEFT, moveX < -0.05f || (p.held & XBOX_PAD_DPAD_LEFT));
	setKey(XBOX_KEY_DPAD_RIGHT, moveX > 0.05f || (p.held & XBOX_PAD_DPAD_RIGHT));
	setKey(XBOX_KEY_A, (p.held & XBOX_PAD_A) != 0);
	setKey(XBOX_KEY_B, (p.held & XBOX_PAD_B) != 0);
	setKey(XBOX_KEY_X, (p.held & XBOX_PAD_X) != 0);
	setKey(XBOX_KEY_Y, (p.held & XBOX_PAD_Y) != 0);
	setKey(XBOX_KEY_BLACK, (p.held & XBOX_PAD_BLACK) != 0);
	setKey(XBOX_KEY_WHITE, (p.held & XBOX_PAD_WHITE) != 0);
	setKey(XBOX_KEY_LEFT_TRIGGER, (p.held & XBOX_PAD_LT) != 0);
	setKey(XBOX_KEY_RIGHT_TRIGGER, (p.held & XBOX_PAD_RT) != 0);
	setKey(XBOX_KEY_LEFT_THUMB, (p.held & XBOX_PAD_LEFT_THUMB) != 0);
	setKey(XBOX_KEY_RIGHT_THUMB, (p.held & XBOX_PAD_RIGHT_THUMB) != 0);

	if (p.pressed & XBOX_PAD_RT) lwjgl::Mouse::detail::pushButton(0, true, 0, 0);
	if (p.released & XBOX_PAD_RT) lwjgl::Mouse::detail::pushButton(0, false, 0, 0);
	if (p.pressed & XBOX_PAD_LT) lwjgl::Mouse::detail::pushButton(1, true, 0, 0);
	if (p.released & XBOX_PAD_LT) lwjgl::Mouse::detail::pushButton(1, false, 0, 0);
	pushKeyEdge(p, XBOX_PAD_START, lwjgl::Keyboard::KEY_ESCAPE);
	if (p.pressed & XBOX_PAD_BLACK) lwjgl::Mouse::detail::pushWheel(-1, 0, 0);
	if (p.pressed & XBOX_PAD_WHITE) lwjgl::Mouse::detail::pushWheel(1, 0, 0);
	pushKeyEdge(p, XBOX_PAD_RIGHT_THUMB, lwjgl::Keyboard::KEY_F5);
	pushKeyEdge(p, XBOX_PAD_BACK, lwjgl::Keyboard::KEY_F3);

	// Gameplay does not use the text/menu latch; keep it from leaking into menus.
	XboxPad::clearLatchedPressed();
}
} // namespace

namespace XboxInput
{

void initialize()
{
	XboxPad::initialize();
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
	XboxPad::poll();
	const XboxPadSnapshot& pad = XboxPad::snapshot();

	if (inMenu && !s_previousMenu)
	{
		releaseGameplayKeys();
		XboxPad::clearLatchedPressed();
		pointerEnterMenu();
	}
	if (!inMenu && s_previousMenu)
	{
		s_suppressedButtons = pad.held;
		s_cameraWarmup = CAMERA_WARMUP_FRAMES;
		lwjgl::Mouse::clearDeltas();
		XboxPad::clearLatchedPressed();
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
			XboxPadSnapshot filtered = pad;
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

} // namespace XboxInput

#endif // XBOX_PLATFORM
