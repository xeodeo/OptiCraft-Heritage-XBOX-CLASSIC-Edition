#pragma once

// Output resolution of the Xbox build, decided once at startup.
//
// The same XBE ships twice in the game folder: default.xbe always runs at
// 640x480, and a copy with "720" in its file name (OptiCraft_720p.xbe) asks
// for 1280x720 progressive. 720p is only used when the dashboard has it
// enabled (component cable, XC_VIDEO_FLAGS_HDTV_720p); otherwise that copy
// falls back to 640x480 as well.
namespace XboxVideoMode
{
int width();
int height();
bool isHd();
}
