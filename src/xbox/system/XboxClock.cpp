#ifdef XBOX_PLATFORM
#include "xbox/system/XboxClock.h"
#include "XboxXtl.h"

namespace
{
uint64_t s_qpcFrequency = 0;
}

uint64_t xboxGetMonotonicMicros()
{
    LARGE_INTEGER counter;
    if (s_qpcFrequency == 0)
    {
        LARGE_INTEGER freq;
        QueryPerformanceFrequency(&freq);
        s_qpcFrequency = static_cast<uint64_t>(freq.QuadPart);
        if (s_qpcFrequency == 0)
            s_qpcFrequency = 1;
    }
    QueryPerformanceCounter(&counter);
    uint64_t ticks = static_cast<uint64_t>(counter.QuadPart);
    return (ticks / s_qpcFrequency) * 1000000ULL + ((ticks % s_qpcFrequency) * 1000000ULL) / s_qpcFrequency;
}
#endif
