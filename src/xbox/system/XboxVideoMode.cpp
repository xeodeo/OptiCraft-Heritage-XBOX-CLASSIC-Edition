#ifdef XBOX_PLATFORM
#include "xbox/system/XboxVideoMode.h"

#include <xtl.h>

#include "platform/Log.h"

// Kernel export: path of the running XBE, e.g.
// "\Device\Harddisk0\Partition1\Games\OptiCraft\OptiCraft_720p.xbe".
struct XboxKernelString
{
    USHORT Length;
    USHORT MaximumLength;
    PCHAR Buffer;
};
extern "C" __declspec(dllimport) XboxKernelString XeImageFileName;

namespace
{
bool imageNameAsks720p()
{
    const XboxKernelString& name = XeImageFileName;
    if (name.Buffer == nullptr)
        return false;
    // Only the file name counts, not the folders above it.
    int start = 0;
    for (int i = 0; i < name.Length; ++i)
        if (name.Buffer[i] == '\\')
            start = i + 1;
    for (int i = start; i + 2 < name.Length; ++i)
        if (name.Buffer[i] == '7' && name.Buffer[i + 1] == '2' && name.Buffer[i + 2] == '0')
            return true;
    return false;
}

struct Mode
{
    int width = 640;
    int height = 480;
    bool hd = false;
};

const Mode& mode()
{
    static Mode s_mode;
    static bool s_decided = false;
    if (!s_decided)
    {
        s_decided = true;
        const bool asked = imageNameAsks720p();
        const DWORD flags = XGetVideoFlags();
        if (asked && (flags & XC_VIDEO_FLAGS_HDTV_720p) != 0)
        {
            s_mode.width = 1280;
            s_mode.height = 720;
            s_mode.hd = true;
        }
        MC_LOG_INFO("xbox", "video: image asks 720p=%d, dashboard flags=%08lx -> %dx%d\n",
                    asked ? 1 : 0, static_cast<unsigned long>(flags), s_mode.width, s_mode.height);
    }
    return s_mode;
}
} // namespace

namespace XboxVideoMode
{
int width() { return mode().width; }
int height() { return mode().height; }
bool isHd() { return mode().hd; }
}

#endif
