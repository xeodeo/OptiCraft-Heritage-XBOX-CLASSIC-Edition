// XboxLogRing.cpp — in-memory copy of the game log for debugging under xemu.
//
// The Xbox has no console to print to, and debug.log lives inside the
// emulator's disk image. Every MC_LOG line is also appended to this ring
// buffer, whose symbol the xemu gdbstub tooling reads straight out of guest
// memory (address from the linker map). Plain C symbols so the map names them
// without decoration beyond the leading underscore.
#ifdef XBOX_PLATFORM

#include <cstring>

extern "C" {
char g_xboxLogRing[64 * 1024];
unsigned long g_xboxLogRingWrite = 0;  // total bytes ever written

void xboxLogRingAppend(const char* line)
{
    if (!line) return;
    const std::size_t size = sizeof(g_xboxLogRing);
    for (const char* p = line; *p; ++p)
    {
        g_xboxLogRing[g_xboxLogRingWrite % size] = *p;
        ++g_xboxLogRingWrite;
    }
}
}

#endif // XBOX_PLATFORM
