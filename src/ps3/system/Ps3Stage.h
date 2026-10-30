#pragma once

// Where the main thread is, for the PS3 frame watchdog (Ps3Rsx.cpp): a few
// scopes in the game loop name themselves, and the watchdog's heartbeat prints
// the innermost one. After a freeze the last heartbeat says where it stopped.
// No-op on every other platform.

#ifdef PS3_PLATFORM
extern "C" const char* volatile g_ps3Stage;
// Free-form numbers a stage can leave for the heartbeat (e.g. an entity's position).
extern "C" volatile double g_ps3StageValues[6];

struct Ps3StageScope
{
    const char* previous;
    explicit Ps3StageScope(const char* name) : previous(g_ps3Stage) { g_ps3Stage = name; }
    ~Ps3StageScope() { g_ps3Stage = previous; }
};
#define PS3_STAGE_SCOPE(name) Ps3StageScope ps3StageScope_(name)

// Who is marking render sections dirty, for the "ps3.dirty" counters in
// RenderGlobal::markRenderersInRange. Main thread only, like the stage.
enum Ps3DirtySource
{
    PS3_DIRTY_OTHER = 0,
    PS3_DIRTY_PUBLISH,     // a chunk column became resident
    PS3_DIRTY_POPULATE,    // decoration wrote into loaded sections
    PS3_DIRTY_SKYLIGHT,    // Chunk::generateSkylightMap and neighbours
    PS3_DIRTY_BLOCK,       // World::notifyBlockChange (setBlock path)
    PS3_DIRTY_SOURCE_COUNT
};
extern int g_ps3DirtySource;
struct Ps3DirtySourceScope
{
    int previous;
    explicit Ps3DirtySourceScope(int source) : previous(g_ps3DirtySource) { g_ps3DirtySource = source; }
    ~Ps3DirtySourceScope() { g_ps3DirtySource = previous; }
};
#define PS3_DIRTY_SOURCE(source) Ps3DirtySourceScope ps3DirtySourceScope_(source)
#else
#define PS3_STAGE_SCOPE(name) ((void)0)
#define PS3_DIRTY_SOURCE(source) ((void)0)
#endif
