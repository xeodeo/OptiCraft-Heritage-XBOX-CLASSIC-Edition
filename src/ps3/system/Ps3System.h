#pragma once

// XMB integration: the sysutil callback (PS button menu, "Quit Game") and the
// exit request it raises. pumpEvents() must run every frame -- a game that stops
// dispatching sysutil callbacks is treated as hung by the system software.
namespace Ps3System
{
void initialize();
void pumpEvents();
bool exitRequested();
}
