#ifdef XBOX_PLATFORM
#include "xbox/system/XboxGameDrive.h"

#include <xtl.h>

#include "platform/Log.h"

namespace
{
// Kernel ANSI_STRING.
struct KernelString
{
    USHORT Length;
    USHORT MaximumLength;
    PCHAR Buffer;
};
} // namespace

// Kernel exports. XeImageFileName is the running XBE as a device path, e.g.
// "\Device\Harddisk0\Partition1\Games\OptiCraft\1.5.2\OptiCraft.xbe".
extern "C" __declspec(dllimport) KernelString XeImageFileName;
extern "C" __declspec(dllimport) LONG __stdcall IoCreateSymbolicLink(KernelString *link, KernelString *device);
extern "C" __declspec(dllimport) LONG __stdcall IoDeleteSymbolicLink(KernelString *link);

namespace XboxGameDrive
{
void mapToImageFolder()
{
    const KernelString &image = XeImageFileName;
    if (image.Buffer == nullptr || image.Length == 0)
        return;
    int slash = -1;
    for (int i = 0; i < image.Length; ++i)
        if (image.Buffer[i] == '\\')
            slash = i;
    if (slash <= 0)
        return;

    char folder[260];
    if (slash >= static_cast<int>(sizeof(folder)))
        return;
    memcpy(folder, image.Buffer, slash);
    folder[slash] = 0;

    char linkName[] = "\\??\\D:";
    KernelString link = {static_cast<USHORT>(sizeof(linkName) - 1), static_cast<USHORT>(sizeof(linkName)), linkName};
    KernelString device = {static_cast<USHORT>(slash), static_cast<USHORT>(sizeof(folder)), folder};
    IoDeleteSymbolicLink(&link);
    const LONG status = IoCreateSymbolicLink(&link, &device);
    MC_LOG_INFO("xbox", "D: -> %s (status %08lx)\n", folder, static_cast<unsigned long>(status));
}
} // namespace XboxGameDrive
#endif
