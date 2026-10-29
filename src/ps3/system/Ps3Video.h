#pragma once

// The output mode the game renders at: whatever the XMB is already set to
// (720p/1080p/480p/576p), except 1080p, which drops to 720p when the TV
// reports 720p (videoGetResolutionAvailability). Never a mode the TV does not
// report -- forcing one (PSL1GHT's rsxtest tries 960x1080) left the real
// console on a black screen.
namespace Ps3Video
{
int width();
int height();
bool isWidescreen();
int resolutionId();  // VIDEO_RESOLUTION_* for videoConfigure
}
