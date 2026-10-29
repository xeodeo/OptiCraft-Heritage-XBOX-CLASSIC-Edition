#ifdef PS3_PLATFORM

#include "ps3/system/Ps3Video.h"

#include <sysutil/video.h>

namespace
{
struct Mode
{
    int width = 1280;
    int height = 720;
    bool widescreen = true;
    int resolutionId = VIDEO_RESOLUTION_720;
};

Mode queryMode()
{
    Mode mode;
    videoState state;
    if (videoGetState(0, 0, &state) != 0)
        return mode;
    mode.resolutionId = state.displayMode.resolution;
    // 1080p is 2.25x the pixels of 720p and the RSX was the bottleneck at it
    // (the CPU waited on the GPU most of each frame). Drop to 720p when the TV
    // reports it; the console scales the picture.
    if (mode.resolutionId == VIDEO_RESOLUTION_1080 &&
        videoGetResolutionAvailability(0, VIDEO_RESOLUTION_720, VIDEO_ASPECT_AUTO, 0) == 1)
        mode.resolutionId = VIDEO_RESOLUTION_720;
    videoResolution resolution;
    if (videoGetResolution(mode.resolutionId, &resolution) == 0)
    {
        mode.width = resolution.width;
        mode.height = resolution.height;
    }
    mode.widescreen = state.displayMode.aspect != VIDEO_ASPECT_4_3;
    return mode;
}

const Mode& currentMode()
{
    static const Mode mode = queryMode();
    return mode;
}
}

namespace Ps3Video
{
int width() { return currentMode().width; }
int height() { return currentMode().height; }
bool isWidescreen() { return currentMode().widescreen; }
int resolutionId() { return currentMode().resolutionId; }
}

#endif // PS3_PLATFORM
