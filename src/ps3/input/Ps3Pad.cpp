// Ps3Pad.cpp — reads the DualShock 3 controller through PSL1GHT's ioPad.
//
// Seven ports; the first one with a controller is player 1, so it works no
// matter which port the pad is assigned to. The next connected controller is
// player 2 (split screen).
#ifdef PS3_PLATFORM

#include <io/pad.h>

#include "ps3/input/Ps3Pad.h"
#include "ps3/system/Ps3Clock.h"
#include "ps3/system/Ps3Paths.h"
#include "ps3/render/Ps3Rsx.h"
#include "platform/Log.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

namespace
{
const int kPorts = 7;

bool s_initialized = false;
Ps3PadSnapshot s_snapshot = {false, 0.0f, 0.0f, 0.0f, 0.0f, 0, 0, 0};
// Per player: [0] = first connected controller, [1] = the second one.
Ps3PadSnapshot s_players[2] = {{false, 0.0f, 0.0f, 0.0f, 0.0f, 0, 0, 0},
                               {false, 0.0f, 0.0f, 0.0f, 0.0f, 0, 0, 0}};
// Whose controller the menu queries (platformTextInputSnapshot and friends)
// read: 0, or 1 while Minecraft is in player 2's screen context. snapshot()
// itself is always player 1.
int s_menuPlayer = 0;
// Presses since the last consume, per player (each player's menus).
unsigned short s_latched[2] = {0, 0};
// L1/R1 presses kept apart until a screen takes them: the D-pad slot
// navigator consumes the menu presses every frame, so a screen that reads
// its tab keys on its 20 Hz tick (the creative inventory) never saw them.
unsigned short s_pageLatched[2] = {0, 0};
const unsigned short kPageButtons = PS3_PAD_L1 | PS3_PAD_R1;

// DualShock 3 analog sticks report 0..255 with 128 at center and 0 = up/left.
// Normalize to -1..1 with Y up negative (matching XboxPad.h).
float normalizeAxis(unsigned int value)
{
	const int centered = static_cast<int>(value & 0xFFu) - 128;
	return centered < 0 ? static_cast<float>(centered) / 128.0f : static_cast<float>(centered) / 127.0f;
}

unsigned short buttonsFrom(const padData& pad)
{
	unsigned short bits = 0;
	if (pad.BTN_UP) bits |= PS3_PAD_DPAD_UP;
	if (pad.BTN_DOWN) bits |= PS3_PAD_DPAD_DOWN;
	if (pad.BTN_LEFT) bits |= PS3_PAD_DPAD_LEFT;
	if (pad.BTN_RIGHT) bits |= PS3_PAD_DPAD_RIGHT;
	if (pad.BTN_START) bits |= PS3_PAD_START;
	if (pad.BTN_SELECT) bits |= PS3_PAD_SELECT;
	if (pad.BTN_L3) bits |= PS3_PAD_L3;
	if (pad.BTN_R3) bits |= PS3_PAD_R3;
	if (pad.BTN_CROSS) bits |= PS3_PAD_CROSS;
	if (pad.BTN_CIRCLE) bits |= PS3_PAD_CIRCLE;
	if (pad.BTN_SQUARE) bits |= PS3_PAD_SQUARE;
	if (pad.BTN_TRIANGLE) bits |= PS3_PAD_TRIANGLE;
	if (pad.BTN_R1) bits |= PS3_PAD_R1;
	if (pad.BTN_L1) bits |= PS3_PAD_L1;
	if (pad.BTN_L2) bits |= PS3_PAD_L2;
	if (pad.BTN_R2) bits |= PS3_PAD_R2;
	return bits;
}

void readPlayer(Ps3PadSnapshot& out, bool connected, const padData* pad)
{
	const unsigned short previous = out.held;
	if (!connected)
	{
		out.connected = false;
		out.leftX = out.leftY = out.rightX = out.rightY = 0.0f;
		out.held = 0;
	}
	else
	{
		if (!out.connected)
		{
			out.leftX = out.leftY = out.rightX = out.rightY = 0.0f;
			out.held = 0;
		}
		out.connected = true;
		// pad->len == 0 means no new report since the last ioPadGetData call;
		// keep the previous sticks and held buttons.
		if (pad != nullptr && pad->len > 0)
		{
			out.leftX = normalizeAxis(pad->ANA_L_H);
			out.leftY = normalizeAxis(pad->ANA_L_V);
			out.rightX = normalizeAxis(pad->ANA_R_H);
			out.rightY = normalizeAxis(pad->ANA_R_V);
			out.held = buttonsFrom(*pad);
		}
	}
	out.pressed = static_cast<unsigned short>(out.held & ~previous);
	out.released = static_cast<unsigned short>(previous & ~out.held);
}

#if PS3_AUTOPILOT
struct AutopilotStep
{
	std::uint32_t start;
	unsigned short buttons;
	float lx, ly, rx, ry;
	unsigned short buttons2;
};
AutopilotStep s_steps[128];
int s_stepCount = -1;  // -1: not loaded yet
std::uint32_t s_autopilotStart = 0;
bool s_autopilotHasP2 = false;
unsigned short s_autopilotP2 = 0;

unsigned short parseButtons(const char* text)
{
	static const struct { const char* name; unsigned short bit; } kNames[] = {
		{"CROSS", PS3_PAD_CROSS}, {"CIRCLE", PS3_PAD_CIRCLE}, {"SQUARE", PS3_PAD_SQUARE}, {"TRIANGLE", PS3_PAD_TRIANGLE},
		{"A", PS3_PAD_CROSS}, {"B", PS3_PAD_CIRCLE}, {"X", PS3_PAD_SQUARE}, {"Y", PS3_PAD_TRIANGLE},
		{"START", PS3_PAD_START}, {"SELECT", PS3_PAD_SELECT}, {"BACK", PS3_PAD_SELECT},
		{"UP", PS3_PAD_DPAD_UP}, {"DOWN", PS3_PAD_DPAD_DOWN}, {"LEFT", PS3_PAD_DPAD_LEFT}, {"RIGHT", PS3_PAD_DPAD_RIGHT},
		{"L2", PS3_PAD_L2}, {"R2", PS3_PAD_R2}, {"LT", PS3_PAD_L2}, {"RT", PS3_PAD_R2},
		{"L1", PS3_PAD_L1}, {"R1", PS3_PAD_R1}, {"WHITE", PS3_PAD_L1}, {"BLACK", PS3_PAD_R1},
		{"L3", PS3_PAD_L3}, {"R3", PS3_PAD_R3}, {"LS", PS3_PAD_L3}, {"RS", PS3_PAD_R3},
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
				if (std::strcmp(token, entry.name) == 0) bits |= entry.bit;
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
	const std::string path = std::string(Ps3Paths::installDir()) + "/autopilot.txt";
	FILE* f = std::fopen(path.c_str(), "r");
	if (!f) return;
	char line[128];
	while (s_stepCount < 128 && std::fgets(line, sizeof(line), f))
	{
		AutopilotStep& step = s_steps[s_stepCount];
		char buttons[64];
		char buttons2[64] = "-";
		unsigned long start = 0;
		step.lx = step.ly = step.rx = step.ry = 0.0f;
		const int fields = std::sscanf(line, "%lu %63s %f %f %f %f %63s", &start, buttons, &step.lx, &step.ly,
		                               &step.rx, &step.ry, buttons2);
		if (fields >= 2)
		{
			step.start = static_cast<std::uint32_t>(start);
			step.buttons = parseButtons(buttons);
			step.buttons2 = parseButtons(buttons2);
			if (fields >= 7)
				s_autopilotHasP2 = true;
			++s_stepCount;
		}
	}
	std::fclose(f);
}

// Remote control (test builds): a PC drops commands into
// /dev_hdd0/tmp/opticraft_mando.txt (RPCS3: tools\rpcs3\dev_hdd0\tmp; console:
// FTP) and the game plays them as if from the pad -- driven live, step by step,
// instead of a timed script. One command per line:
//   <BUTTONS> <ms> [lx ly rx ry]   hold, e.g. "DOWN 200", "A 200", "- 2000 0 -1 0 0"
//   SHOT                           save the next frame (see Ps3Rsx::requestScreenshot)
// The file is deleted once read; each button command is followed by a short
// release so repeated presses register. ps3\tools\mando.py writes it.
struct RemoteCommand
{
	unsigned short buttons;
	float lx, ly, rx, ry;
	std::uint32_t durationMs;
	bool screenshot;
};
RemoteCommand s_remote[64];
int s_remoteCount = 0;
int s_remoteIndex = 0;
std::uint32_t s_remoteEnds = 0;
bool s_remoteActive = false;
int s_remotePolls = 0;

void readRemoteFile()
{
	const char* path = "/dev_hdd0/tmp/opticraft_mando.txt";
	FILE* f = std::fopen(path, "r");
	if (!f) return;
	char line[128];
	int count = 0;
	while (count < 62 && std::fgets(line, sizeof(line), f))
	{
		RemoteCommand& cmd = s_remote[count];
		cmd = RemoteCommand();
		char buttons[64];
		unsigned long ms = 0;
		if (std::strncmp(line, "SHOT", 4) == 0)
		{
			cmd.screenshot = true;
			++count;
			continue;
		}
		const int fields = std::sscanf(line, "%63s %lu %f %f %f %f", buttons, &ms, &cmd.lx, &cmd.ly, &cmd.rx, &cmd.ry);
		if (fields < 2) continue;
		cmd.buttons = parseButtons(buttons);
		cmd.durationMs = static_cast<std::uint32_t>(ms);
		++count;
		if (cmd.buttons != 0 && count < 62)
		{
			RemoteCommand& gap = s_remote[count++];
			gap = RemoteCommand();
			gap.durationMs = 150;
		}
	}
	std::fclose(f);
	std::remove(path);
	s_remoteCount = count;
	s_remoteIndex = 0;
	s_remoteEnds = 0;
	s_remoteActive = true;
	MC_LOG_INFO("ps3.input", "remote: %d commands\n", count);
}

bool remoteState(Ps3PadSnapshot& out)
{
	if (++s_remotePolls % 10 == 0 && (s_remoteIndex >= s_remoteCount))
		readRemoteFile();
	if (!s_remoteActive) return false;
	if (s_remoteIndex >= s_remoteCount)
	{
		// Queue played out: hand the pad back to the real controller.
		s_remoteActive = false;
		return false;
	}
	const std::uint32_t now = ps3GetTicksMs();
	out.connected = true;
	out.leftX = out.leftY = out.rightX = out.rightY = 0.0f;
	out.held = 0;
	while (s_remoteIndex < s_remoteCount)
	{
		RemoteCommand& cmd = s_remote[s_remoteIndex];
		if (cmd.screenshot)
		{
			Ps3Rsx::requestScreenshot();
			++s_remoteIndex;
			continue;
		}
		if (s_remoteEnds == 0)
			s_remoteEnds = now + cmd.durationMs;
		if (now >= s_remoteEnds)
		{
			++s_remoteIndex;
			s_remoteEnds = 0;
			continue;
		}
		out.held = cmd.buttons;
		out.leftX = cmd.lx;
		out.leftY = cmd.ly;
		out.rightX = cmd.rx;
		out.rightY = cmd.ry;
		break;
	}
	return true;
}

bool autopilotState(Ps3PadSnapshot& out)
{
	if (remoteState(out)) return true;
	if (s_stepCount < 0) loadAutopilot();
	if (s_stepCount == 0) return false;
	const std::uint32_t now = ps3GetTicksMs();
	if (s_autopilotStart == 0) s_autopilotStart = now;
	const std::uint32_t elapsed = now - s_autopilotStart;
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

namespace Ps3Pad
{

void initialize()
{
	if (s_initialized)
		return;
	ioPadInit(kPorts);
	s_initialized = true;
}

void poll()
{
	if (!s_initialized)
		initialize();

	const unsigned short previous = s_snapshot.held;
#if PS3_AUTOPILOT
	if (autopilotState(s_snapshot))
	{
		s_snapshot.pressed = static_cast<unsigned short>(s_snapshot.held & ~previous);
		s_snapshot.released = static_cast<unsigned short>(previous & ~s_snapshot.held);
		s_latched[0] |= s_snapshot.pressed;
		s_pageLatched[0] |= s_snapshot.pressed & kPageButtons;
		s_players[0] = s_snapshot;
		const unsigned short previous2 = s_players[1].held;
		s_players[1].connected = s_autopilotHasP2;
		s_players[1].held = s_autopilotP2;
		if (s_players[1].held != previous2)
			MC_LOG_INFO("ps3.input", "autopilot pad 2 held=%04x connected=%d\n", s_players[1].held, s_autopilotHasP2 ? 1 : 0);
		s_players[1].pressed = static_cast<unsigned short>(s_autopilotP2 & ~previous2);
		s_players[1].released = static_cast<unsigned short>(previous2 & ~s_autopilotP2);
		s_latched[1] |= s_players[1].pressed;
		s_pageLatched[1] |= s_players[1].pressed & kPageButtons;
		return;
	}
#endif

	padInfo info;
	std::memset(&info, 0, sizeof(info));
	ioPadGetInfo(&info);

	padData datas[2];
	bool connected[2] = {false, false};
	int found = 0;
	for (int port = 0; port < kPorts && found < 2; ++port)
	{
		if (info.status[port] == 0)
			continue;
		std::memset(&datas[found], 0, sizeof(padData));
		if (ioPadGetData(port, &datas[found]) != 0)
			datas[found].len = 0;
		connected[found] = true;
		++found;
	}
	readPlayer(s_players[0], connected[0], connected[0] ? &datas[0] : nullptr);
	readPlayer(s_players[1], connected[1], connected[1] ? &datas[1] : nullptr);

	const Ps3PadSnapshot& source = s_players[0];
	s_snapshot.connected = source.connected;
	s_snapshot.leftX = source.leftX;
	s_snapshot.leftY = source.leftY;
	s_snapshot.rightX = source.rightX;
	s_snapshot.rightY = source.rightY;
	s_snapshot.held = source.held;
	s_snapshot.pressed = static_cast<unsigned short>(s_snapshot.held & ~previous);
	s_snapshot.released = static_cast<unsigned short>(previous & ~s_snapshot.held);
	if (source.connected)
	{
		s_latched[0] |= s_snapshot.pressed;
		s_pageLatched[0] |= s_snapshot.pressed & kPageButtons;
	}
	else
	{
		s_latched[0] = 0;   // no controller: everything released once, then idle
		s_pageLatched[0] = 0;
	}
	if (s_players[1].connected)
	{
		s_latched[1] |= s_players[1].pressed;
		s_pageLatched[1] |= s_players[1].pressed & kPageButtons;
	}
	else
	{
		s_latched[1] = 0;
		s_pageLatched[1] = 0;
	}
}

const Ps3PadSnapshot& playerSnapshot(int player)
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

const Ps3PadSnapshot& snapshot()
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

unsigned short consumePagePressed()
{
	const unsigned short pressed = s_pageLatched[s_menuPlayer];
	s_pageLatched[s_menuPlayer] = 0;
	return pressed;
}

} // namespace Ps3Pad

#endif // PS3_PLATFORM
