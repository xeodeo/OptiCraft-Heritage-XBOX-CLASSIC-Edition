#include "ps3/system/Ps3Clock.h"

#include <sys/systime.h>

// The PPU timebase (mftb) is a free-running 64-bit counter at a fixed rate
// (79.8 MHz on retail consoles), monotonic and readable from user mode without
// a syscall. lv2 reports the exact rate; it is read once.
namespace
{
std::uint64_t timebaseFrequency()
{
    static const std::uint64_t frequency = []() {
        const std::uint64_t reported = sysGetTimebaseFrequency();
        return reported != 0 ? reported : 79800000ull;
    }();
    return frequency;
}

std::uint64_t timebaseNow()
{
    return __builtin_ppc_get_timebase();
}

std::uint64_t timebaseToMicros(std::uint64_t ticks)
{
    // Split to keep ticks * 1e6 from overflowing after ~2.7 days of uptime.
    const std::uint64_t frequency = timebaseFrequency();
    return (ticks / frequency) * 1000000ull + (ticks % frequency) * 1000000ull / frequency;
}
}

std::uint32_t ps3GetTicksMs()
{
    return static_cast<std::uint32_t>(timebaseToMicros(timebaseNow()) / 1000ull);
}

std::uint64_t ps3GetMonotonicMicros()
{
    return timebaseToMicros(timebaseNow());
}

void ps3SleepMs(std::uint32_t ms)
{
    if (ms != 0)
        sysUsleep(ms * 1000u);
}
