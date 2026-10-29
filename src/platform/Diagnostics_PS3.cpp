#include "platform/Diagnostics.h"
#include "platform/Log.h"
#include "ps3/system/Ps3Memory.h"

const char* platformOomDiagnosticLine(int index)
{
    (void)index;
    return "";
}

void platformMemoryCheckpoint(const char* tag)
{
    const Ps3Memory::Info info = Ps3Memory::query();
    MC_LOG_DEBUG("memory", "%-24s total=%luKB free=%luKB\n",
                 tag != nullptr ? tag : "(null)",
                 (unsigned long)(info.totalBytes / 1024),
                 (unsigned long)(info.availableBytes / 1024));
}

void platformHardwareCheckpoint(const char* tag)
{
    (void)tag;
}

long platformHeapFreeKb()
{
    return static_cast<long>(Ps3Memory::query().availableBytes / 1024);
}

void platformCaptureBadAlloc()
{
    MC_LOG_ERROR("memory", "out of memory: free=%luKB\n",
                 (unsigned long)(Ps3Memory::query().availableBytes / 1024));
}
