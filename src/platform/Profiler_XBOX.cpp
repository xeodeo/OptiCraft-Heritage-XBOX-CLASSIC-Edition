// Profiler_XBOX.cpp — frame-time breakdown for the Xbox, reported in the log.
//
// Render phases are timed with the Pentium III time-stamp counter (733 MHz);
// the world hooks report nanoseconds. Everything accumulates until
// xboxProfileReport() (called from Display::swapBuffers every few seconds)
// logs per-frame averages and resets. Only active with MC_LOG_LEVEL > 0.
#include "platform/Profiler.h"
#include "platform/Log.h"

#include <intrin.h>

namespace
{
constexpr double kCyclesPerMs = 733333.0;   // Xbox CPU clock
constexpr int kRenderPhases = 16;

unsigned long long s_renderCycles[kRenderPhases] = {};
long long s_tickNs = 0;
long long s_meshNs = 0;
long long s_generateNs = 0;
long long s_populateNs = 0;
long long s_chunkLoadNs = 0;
long long s_unloadSaveNs = 0;
int s_meshCount = 0;
int s_generateCount = 0;

const char* const kPhaseNames[kRenderPhases] = {
    "sky", "frustum", "build", "opaque", "entities", "transl", "hand", "hud",
    "entDraw", "tileEnt", "hudItems", "hudText", "hudHints", "p13", "p14", "p15"};
}

std::uint32_t platformProfileRenderPhaseBegin()
{
    return static_cast<std::uint32_t>(__rdtsc());
}

void platformProfileRenderPhaseEnd(std::uint32_t start, PlatformRenderPhase phase)
{
    const int index = static_cast<int>(phase);
    if (index >= 0 && index < kRenderPhases)
        s_renderCycles[index] += static_cast<std::uint32_t>(static_cast<std::uint32_t>(__rdtsc()) - start);
}

void platformProfileTickPhase(const char*, long long ns) { s_tickNs += ns; }
void platformProfileChunkBuild(long long, int) {}
void platformProfileChunkMeshPass(int, long long, int) {}
void platformProfileSnowColumn(bool, bool, int) {}
void platformProfilePopulatePhase(PlatformPopulatePhase, long long) {}
void platformProfileChunkLoad(long long ns) { s_chunkLoadNs += ns; }
void platformProfilePopulate(long long ns) { s_populateNs += ns; }
void platformProfileGenerate(long long ns) { s_generateNs += ns; ++s_generateCount; }
void platformProfileMesh(long long ns) { s_meshNs += ns; ++s_meshCount; }
void platformProfileUnloadSave(long long ns) { s_unloadSaveNs += ns; }
void platformProfileTickUpdates(long long) {}
void platformProfileTickQueue(long long) {}
void platformProfileMobSpawn(long long) {}
void platformProfileSaveWorldInfo(long long) {}
void platformProfileMapStorage(long long) {}
void platformProfileChunkEvict(long long) {}

// Logs the averages since the last report and resets. frames = frames drawn,
// elapsedMs = wall time, presentMs = time spent inside Present() (waiting for
// the GPU / vsync) over the same period.
int xboxClientProfileTake(long long out[4]);   // ClientProfilerBackend_XBOX.cpp

void xboxProfileReport(unsigned int frames, unsigned long elapsedMs, double presentMs)
{
    if (frames == 0 || elapsedMs == 0)
        return;
    long long client[4] = {};
    const int ticks = xboxClientProfileTake(client);
    const double perFrame = 1.0 / static_cast<double>(frames);
    const double toMs = perFrame / kCyclesPerMs;
    MC_LOG_INFO("xbox.perf", "fps=%.1f frame=%.1fms present=%.1fms | sky=%.1f build=%.1f opaque=%.1f ent=%.1f transl=%.1f hand=%.1f hud=%.1f\n",
                frames * 1000.0 / static_cast<double>(elapsedMs),
                static_cast<double>(elapsedMs) * perFrame, presentMs * perFrame,
                s_renderCycles[0] * toMs, s_renderCycles[2] * toMs, s_renderCycles[3] * toMs,
                s_renderCycles[4] * toMs, s_renderCycles[5] * toMs, s_renderCycles[6] * toMs,
                s_renderCycles[7] * toMs);
    MC_LOG_INFO("xbox.perf", "loop per frame: ticks=%.1fms (%.1f tps) lighting=%.1fms render=%.1fms display=%.1fms\n",
                client[0] * perFrame / 1e6, ticks * 1000.0 / static_cast<double>(elapsedMs),
                client[1] * perFrame / 1e6, client[2] * perFrame / 1e6, client[3] * perFrame / 1e6);
    MC_LOG_INFO("xbox.perf", "world per frame: entities=%.1fms mesh=%.1fms(%d) gen=%.1fms(%d) pop=%.1fms load=%.1fms save=%.1fms\n",
                s_tickNs * perFrame / 1e6, s_meshNs * perFrame / 1e6, s_meshCount,
                s_generateNs * perFrame / 1e6, s_generateCount, s_populateNs * perFrame / 1e6,
                s_chunkLoadNs * perFrame / 1e6, s_unloadSaveNs * perFrame / 1e6);
    for (unsigned long long &c : s_renderCycles)
        c = 0;
    s_tickNs = s_meshNs = s_generateNs = s_populateNs = s_chunkLoadNs = s_unloadSaveNs = 0;
    s_meshCount = s_generateCount = 0;
}
