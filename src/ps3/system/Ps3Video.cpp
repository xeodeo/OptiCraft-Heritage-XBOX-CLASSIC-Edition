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
};

Mode queryMode()
{
    Mode mode;
    videoState state;
    if (videoGetState(0, 0, &state) != 0)
        return mode;
    videoResolution resolution;
    if (videoGetResolution(state.displayMode.resolution, &resolution) == 0)
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
}

#endif // PS3_PLATFORM
