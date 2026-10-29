#ifdef PS3_PLATFORM

#include "ps3/system/Ps3Memory.h"

#include <ppu-lv2.h>

namespace Ps3Memory
{
Info query()
{
    // sys_memory_get_user_memory_size(sys_memory_info_t*): { u32 total; u32 available; }.
    struct alignas(8) { std::uint32_t total; std::uint32_t available; } raw = {0, 0};
    lv2syscall1(352, (u64)(&raw));
    Info info;
    info.totalBytes = raw.total;
    info.availableBytes = raw.available;
    return info;
}
}

#endif // PS3_PLATFORM
