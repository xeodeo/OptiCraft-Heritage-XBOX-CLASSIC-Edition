// XboxPad.cpp — reads the Xbox controller through the XDK's XInput.
//
// Four ports; the first one with a controller is player 1, so it works no
// matter which port the pad (or the emulator's binding) uses.
#ifdef XBOX_PLATFORM

#include "xbox/XboxXtl.h"

#include "xbox/input/XboxPad.h"

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
unsigned short s_latched = 0;

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
// next line starts; times count from the first poll.
struct AutopilotStep
{
	DWORD start;
	unsigned short buttons;
	float lx, ly, rx, ry;
};
AutopilotStep s_steps[128];
int s_stepCount = -1;  // -1: not loaded yet
DWORD s_autopilotStart = 0;

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
		unsigned long start = 0;
		step.lx = step.ly = step.rx = step.ry = 0.0f;
		if (sscanf(line, "%lu %63s %f %f %f %f", &start, buttons, &step.lx, &step.ly, &step.rx, &step.ry) >= 2)
		{
			step.start = start;
			step.buttons = parseButtons(buttons);
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
		s_latched |= s_snapshot.pressed;
		return;
	}
#endif
	for (int port = 0; port < kPorts; ++port)
	{
		if (s_handles[port] == NULL)
			continue;
		XINPUT_STATE state;
		if (XInputGetState(s_handles[port], &state) != ERROR_SUCCESS)
			continue;

		const XINPUT_GAMEPAD& pad = state.Gamepad;
		s_snapshot.connected = true;
		s_snapshot.leftX = normalizeAxis(pad.sThumbLX);
		s_snapshot.leftY = -normalizeAxis(pad.sThumbLY);
		s_snapshot.rightX = normalizeAxis(pad.sThumbRX);
		s_snapshot.rightY = -normalizeAxis(pad.sThumbRY);
		s_snapshot.held = buttonsFrom(pad);
		s_snapshot.pressed = static_cast<unsigned short>(s_snapshot.held & ~previous);
		s_snapshot.released = static_cast<unsigned short>(previous & ~s_snapshot.held);
		s_latched |= s_snapshot.pressed;
		return;
	}

	// No controller: report released everything once, then idle.
	s_snapshot.released = previous;
	s_snapshot.connected = false;
	s_snapshot.leftX = s_snapshot.leftY = s_snapshot.rightX = s_snapshot.rightY = 0.0f;
	s_snapshot.held = 0;
	s_snapshot.pressed = 0;
	s_latched = 0;
}

const XboxPadSnapshot& snapshot()
{
	return s_snapshot;
}

unsigned short consumePressed()
{
	const unsigned short pressed = s_latched;
	s_latched = 0;
	return pressed;
}

void clearLatchedPressed()
{
	s_latched = 0;
}

void latchPressed(unsigned short pressed)
{
	s_latched |= pressed;
}

} // namespace XboxPad

#endif // XBOX_PLATFORM
