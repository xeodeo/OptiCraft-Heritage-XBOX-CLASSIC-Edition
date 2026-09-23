#include "platform/Diagnostics.h"
#include "platform/Log.h"
#include "XboxXtl.h"

const char* platformOomDiagnosticLine(int index)
{
    (void)index;
    return "";
}

void platformMemoryCheckpoint(const char* tag)
{
    MEMORYSTATUS ms;
    GlobalMemoryStatus(&ms);
    MC_LOG_DEBUG("memory", "%-24s total=%luKB free=%luKB\n",
                 tag != nullptr ? tag : "(null)",
                 (unsigned long)(ms.dwTotalPhys / 1024),
                 (unsigned long)(ms.dwAvailPhys / 1024));
}

void platformHardwareCheckpoint(const char* tag)
{
    (void)tag;
}

long platformHeapFreeKb()
{
    MEMORYSTATUS ms;
    GlobalMemoryStatus(&ms);
    return static_cast<long>(ms.dwAvailPhys / 1024);
}

void platformCaptureBadAlloc()
{
    MEMORYSTATUS ms;
    GlobalMemoryStatus(&ms);
    MC_LOG_ERROR("memory", "out of memory: free=%luKB\n", (unsigned long)(ms.dwAvailPhys / 1024));
}
