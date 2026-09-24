// ClientProfilerBackend_XBOX.cpp — main-loop timings for the Xbox frame report.
//
// Minecraft::run() reports how long each part of a frame took; these are
// accumulated here and read (and reset) by xboxProfileReport() in
// Profiler_XBOX.cpp, which logs them next to the render-phase breakdown.
#include "platform/ClientProfilerBackend.h"

#include <intrin.h>

void xboxProfileFrameEnd(double frameMs, long long ticksNs, int ticks, long long lightingNs,
                         long long renderNs, long long displayNs);   // Profiler_XBOX.cpp

namespace
{
long long s_ticksNs = 0;
long long s_lightingNs = 0;
long long s_displayNs = 0;
long long s_renderNs = 0;
int s_ticks = 0;

// Current frame only, for the hitch report.
long long s_frameTicksNs = 0;
long long s_frameLightingNs = 0;
long long s_frameDisplayNs = 0;
long long s_frameRenderNs = 0;
int s_frameTicks = 0;
unsigned long long s_lastFrameEnd = 0;
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
void ticks(long long ns, int ticksThisFrame)
{
    s_ticksNs += ns; s_ticks += ticksThisFrame;
    s_frameTicksNs += ns; s_frameTicks += ticksThisFrame;
}
void lighting(long long ns) { s_lightingNs += ns; s_frameLightingNs += ns; }
void displayUpdate(long long ns) { s_displayNs += ns; s_frameDisplayNs += ns; }
void render(long long ns) { s_renderNs += ns; s_frameRenderNs += ns; }
void frameEnd(long long, long long, long long, int, int, World*, RenderGlobal*)
{
    const unsigned long long now = __rdtsc();
    if (s_lastFrameEnd != 0)
        xboxProfileFrameEnd(static_cast<double>(now - s_lastFrameEnd) / 733333.0, s_frameTicksNs, s_frameTicks,
                            s_frameLightingNs, s_frameRenderNs, s_frameDisplayNs);
    s_lastFrameEnd = now;
    s_frameTicksNs = s_frameLightingNs = s_frameDisplayNs = s_frameRenderNs = 0;
    s_frameTicks = 0;
}
}
