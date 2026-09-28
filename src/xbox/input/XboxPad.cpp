// XboxPad.cpp — reads the Xbox controller through the XDK's XInput.
//
// Four ports; the first one with a controller is player 1, so it works no
// matter which port the pad (or the emulator's binding) uses. The next
// connected controller is player 2 (split screen, src/client/XboxSplitScreen).
#ifdef XBOX_PLATFORM

#include "xbox/XboxXtl.h"

#include "xbox/input/XboxPad.h"
#include "platform/Log.h"

#include <cstdio>
#include <cstring>

namespace
{
const int kPorts = 4;
// Analog face buttons and triggers report 0-255; the XDK recommends ~30 as
// the digital threshold for the face buttons.
const BYTE kAnalogPressed = 30;

bool s_initialized = false;
HANDLE s_handles[kPorts] = {};
XboxPadSnapshot s_snapshot = {false, 0.0f, 0.0f, 0.0f, 0.0f, 0, 0, 0};
// Per player: [0] = first connected controller, [1] = the second one.
XboxPadSnapshot s_players[2] = {{false, 0.0f, 0.0f, 0.0f, 0.0f, 0, 0, 0},
                                {false, 0.0f, 0.0f, 0.0f, 0.0f, 0, 0, 0}};
// Whose controller the menu queries (platformTextInputSnapshot and friends)
// read: 0, or 1 while Minecraft is in player 2's screen context. snapshot()
// itself is always player 1.
int s_menuPlayer = 0;
// Presses since the last consume, per player (each player's menus).
unsigned short s_latched[2] = {0, 0};

float normalizeAxis(SHORT value);
unsigned short buttonsFrom(const XINPUT_GAMEPAD& pad);

void readPlayer(XboxPadSnapshot& out, const XINPUT_GAMEPAD* pad)
{
	const unsigned short previous = out.held;
	if (pad == nullptr)
	{
		out.connected = false;
		out.leftX = out.leftY = out.rightX = out.rightY = 0.0f;
		out.held = 0;
	}
	else
	{
		out.connected = true;
		out.leftX = normalizeAxis(pad->sThumbLX);
		out.leftY = -normalizeAxis(pad->sThumbLY);
		out.rightX = normalizeAxis(pad->sThumbRX);
		out.rightY = -normalizeAxis(pad->sThumbRY);
		out.held = buttonsFrom(*pad);
	}
	out.pressed = static_cast<unsigned short>(out.held & ~previous);
	out.released = static_cast<unsigned short>(previous & ~out.held);
}

float normalizeAxis(SHORT value)
{
	return value < 0 ? static_cast<float>(value) / 32768.0f : static_cast<float>(value) / 32767.0f;
}

void openPort(int port)
{
	if (s_handles[port] == NULL)
		s_handles[port] = XInputOpen(XDEVICE_TYPE_GAMEPAD, port, XDEVICE_NO_SLOT, NULL);
}

void closePort(int port)
{
	if (s_handles[port] != NULL)
	{
		XInputClose(s_handles[port]);
		s_handles[port] = NULL;
	}
}

unsigned short buttonsFrom(const XINPUT_GAMEPAD& pad)
{
	unsigned short bits = 0;
	if (pad.wButtons & XINPUT_GAMEPAD_DPAD_UP) bits |= XBOX_PAD_DPAD_UP;
	if (pad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) bits |= XBOX_PAD_DPAD_DOWN;
	if (pad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) bits |= XBOX_PAD_DPAD_LEFT;
	if (pad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) bits |= XBOX_PAD_DPAD_RIGHT;
	if (pad.wButtons & XINPUT_GAMEPAD_START) bits |= XBOX_PAD_START;
	if (pad.wButtons & XINPUT_GAMEPAD_BACK) bits |= XBOX_PAD_BACK;
	if (pad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) bits |= XBOX_PAD_LEFT_THUMB;
	if (pad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) bits |= XBOX_PAD_RIGHT_THUMB;
	if (pad.bAnalogButtons[XINPUT_GAMEPAD_A] > kAnalogPressed) bits |= XBOX_PAD_A;
	if (pad.bAnalogButtons[XINPUT_GAMEPAD_B] > kAnalogPressed) bits |= XBOX_PAD_B;
	if (pad.bAnalogButtons[XINPUT_GAMEPAD_X] > kAnalogPressed) bits |= XBOX_PAD_X;
	if (pad.bAnalogButtons[XINPUT_GAMEPAD_Y] > kAnalogPressed) bits |= XBOX_PAD_Y;
	if (pad.bAnalogButtons[XINPUT_GAMEPAD_BLACK] > kAnalogPressed) bits |= XBOX_PAD_BLACK;
	if (pad.bAnalogButtons[XINPUT_GAMEPAD_WHITE] > kAnalogPressed) bits |= XBOX_PAD_WHITE;
	if (pad.bAnalogButtons[XINPUT_GAMEPAD_LEFT_TRIGGER] > kAnalogPressed) bits |= XBOX_PAD_LT;
	if (pad.bAnalogButtons[XINPUT_GAMEPAD_RIGHT_TRIGGER] > kAnalogPressed) bits |= XBOX_PAD_RT;
	return bits;
}
#if XBOX_AUTOPILOT
// Test autopilot: replays a scripted controller from D:\autopilot.txt so a
// build can be exercised unattended under xemu. One step per line:
//   <start_ms> <buttons|-> <lx> <ly> <rx> <ry>
// buttons: A B X Y START BACK UP DOWN LEFT RIGHT LT RT WHITE BLACK LS RS
// joined with '+'. Sticks are -1..1 (ly/ry: -1 = up). A step holds until the
// next line starts; times count from the first poll. An optional seventh
// field is player 2's buttons (split screen); using it plugs in a pad 2.
struct AutopilotStep
{
	DWORD start;
	unsigned short buttons;
	float lx, ly, rx, ry;
	unsigned short buttons2;
};
AutopilotStep s_steps[128];
int s_stepCount = -1;  // -1: not loaded yet
DWORD s_autopilotStart = 0;
bool s_autopilotHasP2 = false;
unsigned short s_autopilotP2 = 0;

unsigned short parseButtons(const char* text)
{
	static const struct { const char* name; unsigned short bit; } kNames[] = {
		{"A", XBOX_PAD_A}, {"B", XBOX_PAD_B}, {"X", XBOX_PAD_X}, {"Y", XBOX_PAD_Y},
		{"START", XBOX_PAD_START}, {"BACK", XBOX_PAD_BACK}, {"UP", XBOX_PAD_DPAD_UP},
		{"DOWN", XBOX_PAD_DPAD_DOWN}, {"LEFT", XBOX_PAD_DPAD_LEFT}, {"RIGHT", XBOX_PAD_DPAD_RIGHT},
		{"LT", XBOX_PAD_LT}, {"RT", XBOX_PAD_RT}, {"WHITE", XBOX_PAD_WHITE}, {"BLACK", XBOX_PAD_BLACK},
		{"LS", XBOX_PAD_LEFT_THUMB}, {"RS", XBOX_PAD_RIGHT_THUMB},
	};
	unsigned short bits = 0;
	char token[16];
	int n = 0;
	for (const char* p = text;; ++p)
	{
		if (*p == '+' || *p == 0)
		{
			token[n] = 0;
			for (const auto& entry : kNames)
				if (strcmp(token, entry.name) == 0) bits |= entry.bit;
			n = 0;
			if (*p == 0) break;
		}
		else if (n < 15)
		{
			token[n++] = *p;
		}
	}
	return bits;
}

void loadAutopilot()
{
	s_stepCount = 0;
	FILE* f = fopen("D:\\autopilot.txt", "r");
	if (!f) return;
	char line[128];
	while (s_stepCount < 128 && fgets(line, sizeof(line), f))
	{
		AutopilotStep& step = s_steps[s_stepCount];
		char buttons[64];
		char buttons2[64] = "-";
		unsigned long start = 0;
		step.lx = step.ly = step.rx = step.ry = 0.0f;
		const int fields = sscanf(line, "%lu %63s %f %f %f %f %63s", &start, buttons, &step.lx, &step.ly,
		                          &step.rx, &step.ry, buttons2);
		if (fields >= 2)
		{
			step.start = start;
			step.buttons = parseButtons(buttons);
			step.buttons2 = parseButtons(buttons2);
			if (fields >= 7)
				s_autopilotHasP2 = true;
			++s_stepCount;
		}
	}
	fclose(f);
}

bool autopilotState(XboxPadSnapshot& out)
{
	if (s_stepCount < 0) loadAutopilot();
	if (s_stepCount == 0) return false;
	const DWORD now = GetTickCount();
	if (s_autopilotStart == 0) s_autopilotStart = now;
	const DWORD elapsed = now - s_autopilotStart;
	const AutopilotStep* current = nullptr;
	for (int i = 0; i < s_stepCount; ++i)
		if (s_steps[i].start <= elapsed) current = &s_steps[i];
	out.connected = true;
	out.leftX = current ? current->lx : 0.0f;
	out.leftY = current ? current->ly : 0.0f;
	out.rightX = current ? current->rx : 0.0f;
	out.rightY = current ? current->ry : 0.0f;
	out.held = current ? current->buttons : 0;
	s_autopilotP2 = current ? current->buttons2 : 0;
	return true;
}
#endif

} // namespace

namespace XboxPad
{

void initialize()
{
	if (s_initialized)
		return;
	XInitDevices(0, NULL);
	const DWORD present = XGetDevices(XDEVICE_TYPE_GAMEPAD);
	for (int port = 0; port < kPorts; ++port)
	{
		if (present & (1u << port))
			openPort(port);
	}
	s_initialized = true;
}

void poll()
{
	if (!s_initialized)
		initialize();

	DWORD insertions = 0, removals = 0;
	if (XGetDeviceChanges(XDEVICE_TYPE_GAMEPAD, &insertions, &removals))
	{
		for (int port = 0; port < kPorts; ++port)
		{
			if (removals & (1u << port))
				closePort(port);
			if (insertions & (1u << port))
				openPort(port);
		}
	}

	const unsigned short previous = s_snapshot.held;
#if XBOX_AUTOPILOT
	if (autopilotState(s_snapshot))
	{
		s_snapshot.pressed = static_cast<unsigned short>(s_snapshot.held & ~previous);
		s_snapshot.released = static_cast<unsigned short>(previous & ~s_snapshot.held);
		s_latched[0] |= s_snapshot.pressed;
		s_players[0] = s_snapshot;
		const unsigned short previous2 = s_players[1].held;
		s_players[1].connected = s_autopilotHasP2;
		s_players[1].held = s_autopilotP2;
		if (s_players[1].held != previous2)
			MC_LOG_INFO("xbox.input", "autopilot pad 2 held=%04x connected=%d\n", s_players[1].held, s_autopilotHasP2 ? 1 : 0);
		s_players[1].pressed = static_cast<unsigned short>(s_autopilotP2 & ~previous2);
		s_players[1].released = static_cast<unsigned short>(previous2 & ~s_autopilotP2);
		s_latched[1] |= s_players[1].pressed;
		return;
	}
#endif
	XINPUT_STATE states[2];
	int found = 0;
	for (int port = 0; port < kPorts && found < 2; ++port)
	{
		if (s_handles[port] == NULL)
			continue;
		if (XInputGetState(s_handles[port], &states[found]) != ERROR_SUCCESS)
			continue;
		++found;
	}
	readPlayer(s_players[0], found > 0 ? &states[0].Gamepad : nullptr);
	readPlayer(s_players[1], found > 1 ? &states[1].Gamepad : nullptr);

	const XboxPadSnapshot& source = s_players[0];
	s_snapshot.connected = source.connected;
	s_snapshot.leftX = source.leftX;
	s_snapshot.leftY = source.leftY;
	s_snapshot.rightX = source.rightX;
	s_snapshot.rightY = source.rightY;
	s_snapshot.held = source.held;
	s_snapshot.pressed = static_cast<unsigned short>(s_snapshot.held & ~previous);
	s_snapshot.released = static_cast<unsigned short>(previous & ~s_snapshot.held);
	if (source.connected)
		s_latched[0] |= s_snapshot.pressed;
	else
		s_latched[0] = 0;   // no controller: everything released once, then idle
	if (s_players[1].connected)
		s_latched[1] |= s_players[1].pressed;
	else
		s_latched[1] = 0;
}

const XboxPadSnapshot& playerSnapshot(int player)
{
	return s_players[player == 1 ? 1 : 0];
}

void setMenuPlayer(int player)
{
	s_menuPlayer = player == 1 ? 1 : 0;
}

int menuPlayer()
{
	return s_menuPlayer;
}

const XboxPadSnapshot& snapshot()
{
	return s_snapshot;
}

unsigned short consumePressed()
{
	const unsigned short pressed = s_latched[s_menuPlayer];
	s_latched[s_menuPlayer] = 0;
	return pressed;
}

void clearLatchedPressed()
{
	s_latched[s_menuPlayer] = 0;
}

void clearLatchedPressed(int player)
{
	s_latched[player == 1 ? 1 : 0] = 0;
}

void latchPressed(unsigned short pressed)
{
	s_latched[s_menuPlayer] |= pressed;
}

} // namespace XboxPad

#endif // XBOX_PLATFORM
