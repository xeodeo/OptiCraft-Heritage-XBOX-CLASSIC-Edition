// ClientProfilerBackend_XBOX.cpp — main-loop timings for the Xbox frame report.
//
// Minecraft::run() reports how long each part of a frame took; these are
// accumulated here and read (and reset) by xboxProfileReport() in
// Profiler_XBOX.cpp, which logs them next to the render-phase breakdown.
#include "platform/ClientProfilerBackend.h"

namespace
{
long long s_ticksNs = 0;
long long s_lightingNs = 0;
long long s_displayNs = 0;
long long s_renderNs = 0;
int s_ticks = 0;
}

// out: ticks, lighting, render, display update (ns); returns game ticks run.
int xboxClientProfileTake(long long out[4])
{
    out[0] = s_ticksNs;
    out[1] = s_lightingNs;
    out[2] = s_renderNs;
    out[3] = s_displayNs;
    const int ticks = s_ticks;
    s_ticksNs = s_lightingNs = s_displayNs = s_renderNs = 0;
    s_ticks = 0;
    return ticks;
}

namespace ClientProfilerBackend
{
void frameBegin() {}
void ticks(long long ns, int ticksThisFrame) { s_ticksNs += ns; s_ticks += ticksThisFrame; }
void lighting(long long ns) { s_lightingNs += ns; }
void displayUpdate(long long ns) { s_displayNs += ns; }
void render(long long ns) { s_renderNs += ns; }
void frameEnd(long long, long long, long long, int, int, World*, RenderGlobal*) {}
}
