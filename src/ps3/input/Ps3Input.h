#pragma once
#ifdef PS3_PLATFORM

// DualShock 3 front end, the PS3 counterpart of src/xbox/input/XboxInput.h:
// Ps3Pad reads PSL1GHT ioPad, Ps3Input maps it onto the lwjgl keyboard/mouse
// queues the game already understands.
namespace Ps3Input
{
void initialize();
// inMenu / specializedMenuNavigation come from the game, see Display_ps3.cpp.
void poll(bool inMenu, bool specializedMenuNavigation);
// Absolute menu cursor position (container slot navigation uses it).
void setMenuCursor(int x, int y);
// Stick deadzone applied to every analog read (0.05 .. 0.35).
float applyDeadzone(float value);
// Player 1's Select + D-pad combo since the last call: 0 none, 1 save a
// waypoint here (Up), 2 open the waypoint manager (Down).
int consumeWaypointRequest();
}

#endif // PS3_PLATFORM
