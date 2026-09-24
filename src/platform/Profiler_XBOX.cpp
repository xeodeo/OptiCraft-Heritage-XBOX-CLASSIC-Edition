// Profiler_XBOX.cpp — frame-time breakdown for the Xbox, reported in the log.
//
// Render phases are timed with the Pentium III time-stamp counter (733 MHz);
// the world hooks report nanoseconds. Everything accumulates until
// xboxProfileReport() (called from Display::swapBuffers every few seconds)
// logs per-frame averages and resets. Only active with MC_LOG_LEVEL > 0.
#include "platform/Profiler.h"
#include "platform/Log.h"

#include <cstdio>
#include <cstring>
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

// The same, for the current frame only (spike report).
unsigned long long s_frameRenderCycles[kRenderPhases] = {};
long long s_frameTickNs = 0;
long long s_frameGenerateNs = 0;
long long s_framePopulateNs = 0;
long long s_frameChunkLoadNs = 0;
long long s_frameUnloadSaveNs = 0;
}

std::uint32_t platformProfileRenderPhaseBegin()
{
    return static_cast<std::uint32_t>(__rdtsc());
}

void platformProfileRenderPhaseEnd(std::uint32_t start, PlatformRenderPhase phase)
{
    const int index = static_cast<int>(phase);
    if (index >= 0 && index < kRenderPhases)
    {
        const std::uint32_t cycles = static_cast<std::uint32_t>(static_cast<std::uint32_t>(__rdtsc()) - start);
        s_renderCycles[index] += cycles;
        s_frameRenderCycles[index] += cycles;
    }
}

// Tick phases by name for the hitch report. Phases nested inside "entities"
// and "worldTick" are listed on their own but not added to the frame's world
// total again (they used to be counted twice).
namespace
{
constexpr int kTickPhaseSlots = 32;
const char* s_phaseNames[kTickPhaseSlots] = {};
long long s_framePhaseNs[kTickPhaseSlots] = {};
int s_phaseCount = 0;

bool isNestedTickPhase(const char* name)
{
    static const char* const nested[] = {
        "entWeather", "entUnload", "entTick", "entTile",
        "worldWeather", "mobSpawn", "chunkUnload", "skylightChange", "autosave",
        "blockUpdates", "randomBlocks", "villages"};
    for (const char* n : nested)
        if (std::strcmp(n, name) == 0)
            return true;
    return false;
}

void addFramePhase(const char* name, long long ns)
{
    for (int i = 0; i < s_phaseCount; ++i)
        if (s_phaseNames[i] == name || std::strcmp(s_phaseNames[i], name) == 0)
        {
            s_framePhaseNs[i] += ns;
            return;
        }
    if (s_phaseCount < kTickPhaseSlots)
    {
        s_phaseNames[s_phaseCount] = name;
        s_framePhaseNs[s_phaseCount++] = ns;
    }
}
}

void platformProfileTickPhase(const char* name, long long ns)
{
    const bool nested = name != nullptr && isNestedTickPhase(name);
    if (!nested)
    {
        s_tickNs += ns;
        s_frameTickNs += ns;
    }
    if (name != nullptr)
        addFramePhase(name, ns);
}
void platformProfileChunkBuild(long long, int) {}
void platformProfileChunkMeshPass(int, long long, int) {}
void platformProfileSnowColumn(bool, bool, int) {}
void platformProfilePopulatePhase(PlatformPopulatePhase, long long) {}
void platformProfileChunkLoad(long long ns) { s_chunkLoadNs += ns; s_frameChunkLoadNs += ns; }
void platformProfilePopulate(long long ns) { s_populateNs += ns; s_framePopulateNs += ns; }
void platformProfileGenerate(long long ns) { s_generateNs += ns; ++s_generateCount; s_frameGenerateNs += ns; }
void platformProfileMesh(long long ns) { s_meshNs += ns; ++s_meshCount; }
void platformProfileUnloadSave(long long ns) { s_unloadSaveNs += ns; s_frameUnloadSaveNs += ns; }
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

void xboxRenderTakeDrawStats(double* drawMs, long* vbDraws, long* upDraws, long* upVertices, long* viewSets);
void xboxRenderTakeListStats(double* transformMs, double* listMs, double* stageMs, long* listCalls);
void pcLegacyTakeMeshSchedulerStats(long *calls, long *pending, long *steps, long *published, long *stepUs);
void xboxTakeWorldDirtyStats();

// Render parts outside the phase list, per frame (EntityRenderer.cpp):
// 0 particles, 1 rain/snow, 2 clouds, 3 open menu screen.
namespace
{
constexpr int kFrameSlots = 4;
unsigned long long s_frameSlotCycles[kFrameSlots] = {};
}

extern "C" void xboxProfileFrameSlot(int slot, unsigned long long cycles)
{
    if (slot >= 0 && slot < kFrameSlots)
        s_frameSlotCycles[slot] += cycles;
}

// Called once per frame (ClientProfilerBackend_XBOX.cpp). A frame over 45 ms
// is a visible hitch: log where its time went, then start the next frame.
void xboxProfileFrameEnd(double frameMs, long long ticksNs, int ticks, long long lightingNs,
                         long long renderNs, long long displayNs)
{
    if (frameMs > 45.0)
    {
        const double c = 1.0 / kCyclesPerMs;
        // The three most expensive named tick phases of this frame.
        int top[3] = {-1, -1, -1};
        for (int i = 0; i < s_phaseCount; ++i)
        {
            if (top[0] < 0 || s_framePhaseNs[i] > s_framePhaseNs[top[0]]) { top[2] = top[1]; top[1] = top[0]; top[0] = i; }
            else if (top[1] < 0 || s_framePhaseNs[i] > s_framePhaseNs[top[1]]) { top[2] = top[1]; top[1] = i; }
            else if (top[2] < 0 || s_framePhaseNs[i] > s_framePhaseNs[top[2]]) { top[2] = i; }
        }
        char phases[160] = "";
        int used = 0;
        for (int t = 0; t < 3 && top[t] >= 0; ++t)
            used += std::snprintf(phases + used, sizeof(phases) - used, "%s%s=%.1f", t ? " " : "",
                                  s_phaseNames[top[t]], s_framePhaseNs[top[t]] / 1e6);
        MC_LOG_INFO("xbox.spike", "frame=%.0fms ticks=%.1fms(%d) world=%.1fms {%s} gen=%.1fms pop=%.1fms load=%.1fms save=%.1fms light=%.1fms render=%.1fms [build=%.1f opaque=%.1f ent=%.1f hud=%.1f sky=%.1f transl=%.1f hand=%.1f particles=%.1f weather=%.1f clouds=%.1f screen=%.1f] display=%.1fms\n",
                    frameMs, ticksNs / 1e6, ticks, s_frameTickNs / 1e6, phases, s_frameGenerateNs / 1e6,
                    s_framePopulateNs / 1e6, s_frameChunkLoadNs / 1e6, s_frameUnloadSaveNs / 1e6,
                    lightingNs / 1e6, renderNs / 1e6, s_frameRenderCycles[2] * c, s_frameRenderCycles[3] * c,
                    s_frameRenderCycles[4] * c, s_frameRenderCycles[7] * c, s_frameRenderCycles[0] * c,
                    s_frameRenderCycles[5] * c, s_frameRenderCycles[6] * c, s_frameSlotCycles[0] * c,
                    s_frameSlotCycles[1] * c, s_frameSlotCycles[2] * c, s_frameSlotCycles[3] * c, displayNs / 1e6);
    }
    for (int i = 0; i < s_phaseCount; ++i)
        s_framePhaseNs[i] = 0;
    for (int i = 0; i < kFrameSlots; ++i)
        s_frameSlotCycles[i] = 0;
    for (int i = 0; i < kRenderPhases; ++i)
        s_frameRenderCycles[i] = 0;
    s_frameTickNs = s_frameGenerateNs = s_framePopulateNs = s_frameChunkLoadNs = s_frameUnloadSaveNs = 0;
}

// Ad-hoc timing slots for the terrain pass (RenderGlobal), Xbox only.
namespace
{
constexpr int kXboxSlots = 4;
unsigned long long s_xboxSlotCycles[kXboxSlots] = {};
}

extern "C" void xboxProfileSlotAdd(int slot, unsigned long long cycles)
{
    if (slot >= 0 && slot < kXboxSlots)
        s_xboxSlotCycles[slot] += cycles;
}

void xboxProfileReport(unsigned int frames, unsigned long elapsedMs, double presentMs)
{
    if (frames == 0 || elapsedMs == 0)
        return;
    {
        const double slotToMs = 1.0 / (kCyclesPerMs * frames);
        MC_LOG_INFO("xbox.terrain", "per frame: visibility=%.2fms sort=%.2fms select=%.2fms submit=%.2fms\n",
                    s_xboxSlotCycles[0] * slotToMs, s_xboxSlotCycles[1] * slotToMs,
                    s_xboxSlotCycles[2] * slotToMs, s_xboxSlotCycles[3] * slotToMs);
        for (int i = 0; i < kXboxSlots; ++i)
            s_xboxSlotCycles[i] = 0;
    }
    xboxTakeWorldDirtyStats();
    {
        long calls = 0, pending = 0, steps = 0, published = 0, stepUs = 0;
        pcLegacyTakeMeshSchedulerStats(&calls, &pending, &steps, &published, &stepUs);
        const long c = calls > 0 ? calls : 1;
        MC_LOG_INFO("xbox.mesh", "per frame: queue=%ld steps=%ld (%.2fms) | sections published in %.1fs: %ld\n",
                    pending / c, steps / static_cast<long>(frames), stepUs / 1000.0 / frames,
                    elapsedMs / 1000.0, published);
    }
    {
        double transformMs = 0.0, listMs = 0.0, stageMs = 0.0;
        long listCalls = 0;
        xboxRenderTakeListStats(&transformMs, &listMs, &stageMs, &listCalls);
        MC_LOG_INFO("xbox.draw", "per frame: lists=%ld replay=%.2fms setTransform=%.2fms textureStage=%.2fms\n",
                    listCalls / static_cast<long>(frames), listMs / frames, transformMs / frames, stageMs / frames);
    }
    {
        double drawMs = 0.0;
        long vbDraws = 0, upDraws = 0, upVertices = 0, viewSets = 0;
        xboxRenderTakeDrawStats(&drawMs, &vbDraws, &upDraws, &upVertices, &viewSets);
        MC_LOG_INFO("xbox.draw", "per frame: vbDraws=%ld (in D3D %.2fms) upDraws=%ld upVerts=%ld viewSets=%ld\n",
                    vbDraws / static_cast<long>(frames), drawMs / frames, upDraws / static_cast<long>(frames),
                    upVertices / static_cast<long>(frames), viewSets / static_cast<long>(frames));
    }
    long long client[4] = {};
    const int ticks = xboxClientProfileTake(client);
    const double perFrame = 1.0 / static_cast<double>(frames);
    const double toMs = perFrame / kCyclesPerMs;
    MC_LOG_INFO("xbox.perf", "fps=%.1f frame=%.1fms present=%.1fms | sky=%.1f frustum=%.1f build=%.1f opaque=%.1f ent=%.1f transl=%.1f hand=%.1f hud=%.1f\n",
                frames * 1000.0 / static_cast<double>(elapsedMs),
                static_cast<double>(elapsedMs) * perFrame, presentMs * perFrame,
                s_renderCycles[0] * toMs, s_renderCycles[1] * toMs, s_renderCycles[2] * toMs, s_renderCycles[3] * toMs,
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
