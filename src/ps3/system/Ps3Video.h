#pragma once

// The output mode the game renders at: whatever the XMB is already set to
// (720p/1080p/480p/576p). The game never switches modes -- forcing one the TV
// does not report (PSL1GHT's rsxtest tries 960x1080) left the real console on
// a black screen.
namespace Ps3Video
{
int width();
int height();
bool isWidescreen();
}
