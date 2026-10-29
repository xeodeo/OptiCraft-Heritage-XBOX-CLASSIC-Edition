#pragma once

// PS3 time services for the shared platform layer (PlatformCompat.h, System.cpp).
// Implemented in src/ps3/system/Ps3Clock.cpp on top of the PPU timebase
// (sys_time_get_timebase_frequency / __mftb), which is monotonic and does not
// go through newlib's gettimeofday.

#include <cstdint>

std::uint32_t ps3GetTicksMs();
std::uint64_t ps3GetMonotonicMicros();
void ps3SleepMs(std::uint32_t ms);
