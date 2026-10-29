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
#else
#define PS3_STAGE_SCOPE(name) ((void)0)
#endif
