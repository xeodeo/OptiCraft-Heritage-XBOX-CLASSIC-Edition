#pragma once
#ifdef XBOX_PLATFORM

// Xbox controller front end: XboxPad reads XInput, XboxInput maps it onto the
// lwjgl keyboard/mouse queues the game already understands. Same split and
// mapping logic as src/ps2/input (the PS2 pad also has two sticks).
namespace XboxInput
{
void initialize();
// inMenu / specializedMenuNavigation come from the game, see Display_xbox.cpp.
void poll(bool inMenu, bool specializedMenuNavigation);
// Absolute menu cursor position (container slot navigation uses it).
void setMenuCursor(int x, int y);
// Stick deadzone applied to every analog read (0.05 .. 0.35).
float applyDeadzone(float value);
// Player 1's Back + D-pad combo since the last call: 0 none, 1 save a
// waypoint here (Up), 2 open the waypoint manager (Down).
int consumeWaypointRequest();
}

#endif // XBOX_PLATFORM
