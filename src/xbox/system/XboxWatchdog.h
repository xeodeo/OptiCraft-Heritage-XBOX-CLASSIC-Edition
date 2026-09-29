#pragma once
#include <cstdint>
#include "java/Type.h"

#if MC_LOG_LEVEL > 0

extern const char* volatile g_mainPhase;
extern const char* volatile g_mainSubphase;
extern volatile int_t g_mainFrameCount;
extern volatile int_t g_mainPendingCount;

void XboxWatchdog_Init();

struct XboxWatchdog_Phase {
    const char* oldPhase;
    const char* oldSubphase;
    XboxWatchdog_Phase(const char* phase, const char* subphase = "-") {
        oldPhase = g_mainPhase;
        oldSubphase = g_mainSubphase;
        g_mainPhase = phase;
        g_mainSubphase = subphase;
    }
    ~XboxWatchdog_Phase() {
        g_mainPhase = oldPhase;
        g_mainSubphase = oldSubphase;
    }
};

#define XBOX_WATCHDOG_PHASE(phase) XboxWatchdog_Phase _wd_phase(phase)
#define XBOX_WATCHDOG_SUBPHASE(phase, subphase) XboxWatchdog_Phase _wd_phase(phase, subphase)

#else

#define XboxWatchdog_Init() do {} while(0)
#define XBOX_WATCHDOG_PHASE(phase)
#define XBOX_WATCHDOG_SUBPHASE(phase, subphase)

#endif
