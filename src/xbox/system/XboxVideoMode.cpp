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
// True when the running XBE's file name (not the folders above it) contains
// the given digits.
bool imageNameContains(const char* digits)
{
    const XboxKernelString& name = XeImageFileName;
    if (name.Buffer == nullptr)
        return false;
    int start = 0;
    for (int i = 0; i < name.Length; ++i)
        if (name.Buffer[i] == '\\')
            start = i + 1;
    for (int i = start; i < name.Length; ++i)
    {
        int k = 0;
        while (digits[k] != '\0' && i + k < name.Length && name.Buffer[i + k] == digits[k])
            ++k;
        if (digits[k] == '\0')
            return true;
    }
    return false;
}

struct Mode
{
    int width = 640;
    int height = 480;
    bool hd = false;
    bool interlaced = false;
};

const Mode& mode()
{
    static Mode s_mode;
    static bool s_decided = false;
    if (!s_decided)
    {
        s_decided = true;
        const bool asks1080i = imageNameContains("1080");
        const bool asks720p = !asks1080i && imageNameContains("720");
        const DWORD flags = XGetVideoFlags();
        if (asks1080i && (flags & XC_VIDEO_FLAGS_HDTV_1080i) != 0)
        {
            s_mode.width = 1920;
            s_mode.height = 1080;
            s_mode.hd = true;
            s_mode.interlaced = true;
        }
        else if ((asks720p || asks1080i) && (flags & XC_VIDEO_FLAGS_HDTV_720p) != 0)
        {
            // The 1080i copy falls back to 720p when only that is enabled.
            s_mode.width = 1280;
            s_mode.height = 720;
            s_mode.hd = true;
        }
        MC_LOG_INFO("xbox", "video: image asks 720p=%d 1080i=%d, dashboard flags=%08lx -> %dx%d%s\n",
                    asks720p ? 1 : 0, asks1080i ? 1 : 0, static_cast<unsigned long>(flags),
                    s_mode.width, s_mode.height, s_mode.interlaced ? "i" : "");
    }
    return s_mode;
}
} // namespace

namespace XboxVideoMode
{
int width() { return mode().width; }
int height() { return mode().height; }
bool isHd() { return mode().hd; }
bool isInterlaced() { return mode().interlaced; }
}

#endif
