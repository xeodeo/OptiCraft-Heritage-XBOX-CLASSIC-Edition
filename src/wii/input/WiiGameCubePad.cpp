#ifdef WII_PLATFORM

#include <gccore.h>

#include "lwjgl/Keyboard.h"
#include "lwjgl/Mouse.h"
#include "platform/Input.h"
#include "wii/input/WiiButtonBindings.h"
#include "wii/input/WiiGameCubePad.h"
#include "wii/input/WiiInputDebug.h"
#include "wii/input/WiiPadInternal.h"
#include "wii/input/WiiPadKeyCodes.h"
#include "wii/input/WiiPointer.h"

namespace WiiGameCubePad
{

using namespace WiiPadInternal;

namespace
{
bool g_menuStickWasActive = false;
}

void poll(u32 connectedMask, WiiPadInternal::FrameState& state)
{
	WiiInputDebugState& debug = wiiInputDebugState();

	if (!(connectedMask & 1))
	{
		g_menuStickWasActive = false;
		return;
	}

	const u16 held = PAD_ButtonsHeld(0);
	const u16 down = PAD_ButtonsDown(0);

	debug.gcConnected = true;
	debug.gcButtons   = held;
	debug.gcStickX    = PAD_StickX(0);
	debug.gcStickY    = PAD_StickY(0);

	if (platformPadRebindExclusive())
	{
		// GuiWiiControls is listening for a new binding. Report exactly one
		// pressed face button as a synthetic key-down event instead of the
		// button's normal job; the capture flow filters by family, so a press
		// here is ignored unless a GameCube slot is the one being edited.
		static const struct { u16 mask; int code; } kRemappable[] = {
			{ PAD_BUTTON_A, WII_KEY_GC_A }, { PAD_BUTTON_B, WII_KEY_GC_B },
			{ PAD_BUTTON_X, WII_KEY_GC_X }, { PAD_BUTTON_Y, WII_KEY_GC_Y },
			{ PAD_TRIGGER_Z, WII_KEY_GC_Z }, { PAD_TRIGGER_L, WII_KEY_GC_L },
			{ PAD_TRIGGER_R, WII_KEY_GC_R },
		};
		for (const auto &entry : kRemappable)
		{
			if (down & entry.mask)
			{
				lwjgl::Keyboard::detail::pushKey(entry.code, true);
				lwjgl::Keyboard::detail::pushKey(entry.code, false);
				break;
			}
		}
		return;
	}

	const WiiButtonBindings::FamilySnapshot &buttons = WiiButtonBindings::get().gameCube;
	state.setKey(VK_JUMP,        (held & buttons.jump) != 0);
	state.setKey(VK_SNEAK,       (held & buttons.sneak) != 0);
	state.setKey(VK_INVENTORY,   (held & buttons.inventory) != 0);
	state.setKey(VK_DROP,        (held & buttons.drop) != 0);
	state.setKey(VK_ESCAPE,      (held & PAD_BUTTON_START) != 0);

	// L+R opens chat. Suppress both mouse actions while the chord is held so the
	// same press neither places nor breaks a block. Trigger only when the second
	// shoulder goes down; holding the chord cannot repeatedly reopen the screen.
	const bool gcChatChord = (held & (PAD_TRIGGER_L | PAD_TRIGGER_R)) ==
	                         (PAD_TRIGGER_L | PAD_TRIGGER_R);
	if (gcChatChord && (down & (PAD_TRIGGER_L | PAD_TRIGGER_R)) &&
	    lwjgl::Mouse::isGrabbed() && !platformTextInputExclusive())
	{
		lwjgl::Keyboard::detail::pushKey(lwjgl::Keyboard::KEY_T, true);
		lwjgl::Keyboard::detail::pushKey(lwjgl::Keyboard::KEY_T, false);
	}

	// Text-entry bindings for a GameCube pad.  They are collected separately
	// from the normal game bindings and only consumed while VirtualKeyboard is
	// active, so A/B/X/Y/Z keep their ordinary in-game meanings otherwise.
	if (held & PAD_BUTTON_A)     state.textInputHeld |= WII_TEXT_TYPE;
	if (held & PAD_BUTTON_B)     state.textInputHeld |= WII_TEXT_BACK;
	if (held & PAD_BUTTON_X)     state.textInputHeld |= WII_TEXT_SPACE;
	if (held & PAD_TRIGGER_Z)    state.textInputHeld |= WII_TEXT_SHIFT;
	if (held & PAD_BUTTON_START) state.textInputHeld |= WII_TEXT_ENTER;
	if (held & PAD_BUTTON_Y)     state.textInputHeld |= WII_TEXT_CLOSE;
	if (held & PAD_BUTTON_UP)    state.textInputHeld |= WII_TEXT_UP;
	if (held & PAD_BUTTON_DOWN)  state.textInputHeld |= WII_TEXT_DOWN;
	if (held & PAD_BUTTON_LEFT)  state.textInputHeld |= WII_TEXT_LEFT;
	if (held & PAD_BUTTON_RIGHT) state.textInputHeld |= WII_TEXT_RIGHT;
	if (held & PAD_TRIGGER_L)    state.textInputHeld |= WII_TEXT_PREV_PAGE;
	if (held & PAD_TRIGGER_R)    state.textInputHeld |= WII_TEXT_NEXT_PAGE;

	// Z is the debug overlay (F3). The FPS / position / chunk readout is the
	// main way to see what the port is doing without a host log, same reason
	// the PS2 build spends its Select button on it (src/ps2/main_ps2.cpp). It
	// sat on D-pad up for a while, but up/down are now ThirdPerson/Sneak (see
	// WiiButtonBindings.cpp), so Z is the one button left with nothing on it.
	state.setKey(VK_DEBUG, lwjgl::Mouse::isGrabbed() && (held & PAD_TRIGGER_Z) != 0);
	// D-pad up/down keep their arrow-key reading on top of ThirdPerson (F5) and
	// Sneak: Beta 1.7.3 binds nothing to the arrows in game, and in a menu
	// preparePadMenuState() drops everything but Escape/Debug anyway.
	state.setKey(VK_ARROW_UP,    (held & PAD_BUTTON_UP) != 0);
	state.setKey(VK_ARROW_DOWN,  (held & PAD_BUTTON_DOWN) != 0);
	state.setKey(VK_THIRDPERSON, (held & buttons.thirdPerson) != 0);
	state.setKey(VK_ARROW_LEFT,  (held & PAD_BUTTON_LEFT) != 0);
	state.setKey(VK_ARROW_RIGHT, (held & PAD_BUTTON_RIGHT) != 0);

	// Gameplay keeps the configured use binding. In a GUI, physical B is the
	// secondary click so pointer-driven containers match the D-pad split-stack
	// action and the PS2 cursor scheme.
	state.setMouse(VM_USE, lwjgl::Mouse::isGrabbed()
		? (!gcChatChord && (held & buttons.use) != 0)
		: (held & PAD_BUTTON_B) != 0);
	// In a GUI (mouse not grabbed) the click button is always physical A,
	// regardless of what Attack is rebound to for gameplay -- same convention
	// as every other menu confirm button in the game (see PAD_CROSS on PS2).
	// Rebinding Attack to, say, Y must not also move the menu click button.
	state.setMouse(VM_ATTACK, lwjgl::Mouse::isGrabbed()
		? (!gcChatChord && (held & buttons.attack) != 0)
		: (held & PAD_BUTTON_A) != 0);

	// D-pad left/right double as hotbar prev/next. Safe to bind on top of the
	// arrow keys: Beta 1.7.3 binds nothing to the arrows in game, and in a menu
	// the wheel scrolls a list, which is the useful reading of the same press.
	// Without this there is no way to change the selected item on a GameCube
	// pad, which is the difference between "supported" and "playable".
	if (down & PAD_BUTTON_RIGHT) state.wheel -= 1;
	if (down & PAD_BUTTON_LEFT)  state.wheel += 1;

	// PAD_Stick* report roughly -100..100 at the edge of the gate.
	const float lx = state.applyDeadzone(PAD_StickX(0)    / 100.0f);
	const float ly = state.applyDeadzone(PAD_StickY(0)    / 100.0f);
	const float rx = state.applyDeadzone(PAD_SubStickX(0) / 100.0f);
	const float ry = state.applyDeadzone(PAD_SubStickY(0) / 100.0f);
	const bool menuStickActive = lx != 0.0f || ly != 0.0f || rx != 0.0f || ry != 0.0f;
	if (down != 0 || (menuStickActive && !g_menuStickWasActive))
		state.menuPadActivity = true;
	g_menuStickWasActive = menuStickActive;

	state.moveX += lx;
	state.moveY += ly;
	state.menuScrollY = ry;
	// In a menu the C-stick only scrolls lists (menuScrollY above); the cursor
	// is the left stick's job there, see WiiPadState's pad cursor mode.
	if (!state.inMenu)
		WiiPointer::addStickLook(rx, -ry);   // stick Y is up-positive; screen Y is down-positive
}

} // namespace WiiGameCubePad

#endif // WII_PLATFORM
