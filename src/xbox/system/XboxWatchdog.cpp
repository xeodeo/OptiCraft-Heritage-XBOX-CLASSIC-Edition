#include "XboxWatchdog.h"

#if MC_LOG_LEVEL > 0

#include "platform/Log.h"
#include "platform/PlatformCompat.h"
#include "platform/Diagnostics.h"
#include "platform/Thread.h"

const char* volatile g_mainPhase = "init";
const char* volatile g_mainSubphase = "init";
volatile int_t g_mainFrameCount = 0;
volatile int_t g_mainPendingCount = 0;

static PlatformThread s_watchdogThread;

static void* WatchdogThreadFunc(void*)
{
    int_t lastFrame = g_mainFrameCount;
    int stuckTimeMs = 0;
    bool stuck = false;

    while (true)
    {
        PlatformCompat::delay(500);
        int_t currentFrame = g_mainFrameCount;
        
        // Do not warn during early frames or if there's no world (loading)
        bool isLoading = (currentFrame < 100);

        if (currentFrame == lastFrame && !isLoading)
        {
            stuckTimeMs += 500;
            if (stuckTimeMs == 2000)
            {
                stuck = true;
                MC_LOG_ERROR("xbox.watchdog", "main stuck 2000ms phase=%s subphase=%s frame=%d pending=%d freeKB=%ld\n",
                    g_mainPhase, g_mainSubphase, (int)currentFrame, (int)g_mainPendingCount, platformHeapFreeKb());
            }
            else if (stuck && (stuckTimeMs - 2000) % 5000 == 0)
            {
                MC_LOG_ERROR("xbox.watchdog", "main still stuck %dms phase=%s subphase=%s frame=%d pending=%d freeKB=%ld\n",
                    stuckTimeMs, g_mainPhase, g_mainSubphase, (int)currentFrame, (int)g_mainPendingCount, platformHeapFreeKb());
            }
        }
        else
        {
            lastFrame = currentFrame;
            stuckTimeMs = 0;
            stuck = false;
        }
    }
    return nullptr;
}

void XboxWatchdog_Init()
{
    // Priority > 64 is ABOVE_NORMAL on Win32. 80 is high enough.
    s_watchdogThread.start(WatchdogThreadFunc, nullptr, 32 * 1024, 80);
}

#endif
