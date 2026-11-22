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
// Back + D-pad combo per player since the last call:
// 0 none, 1 save waypoint (Up), 2 waypoint manager (Down),
// 3 cycle zoom (Left), 4 minimap options menu (Right).
int consumeWaypointRequest(int playerIndex = 0);
void setWaypointRequest(int playerIndex, int request);
// Back + Y combo for Village Locator mod.
bool consumeVillageLocatorRequest();
}

#endif // XBOX_PLATFORM
