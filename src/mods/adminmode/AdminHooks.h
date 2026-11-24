#pragma once

// Cheap flags read by the engine hooks of the "Admin mode (testing)" mod
// (encargo 122, Nodus 1.5.2 modules). AdminModeMod recomputes g_on[] every
// tick as: mod enabled && in a world && module switched on (any world or
// server, no op needed). Everything is false otherwise, so the
// hooks cost one array read when the mod is off.

class Entity;

namespace AdminHooks
{
enum Module
{
    Aimbot = 0,
    AutoEgg,
    AutoEnchant,
    AutoFish,
    AutoMine,
    AutoSign,
    AutoTool,
    AutoWalk,
    BlockESP,
    BowAimbot,
    Build,
    CaveFinder,
    ChestFinder,
    ClickAimbot,
    Day,
    Derp,
    DisableNametags,
    Dupe,
    ESP,
    FastBreak,
    FastPlace,
    Flood,
    Fly,
    Forcefield,
    Freecam,
    FullBright,
    Health,
    HighJump,
    NoFall,
    NoFlinch,
    NoRender,
    NoSwing,
    Notifications,
    Nuker,
    NoSlowDown,
    Sneak,
    Spider,
    Sprint,
    Step,
    Timer,
    Tracers,
    WallHack,
    Water,
    Weather,
    COUNT
};

extern bool g_on[COUNT];
extern bool g_any;
extern Entity *g_local[2];
// Entity whose collision boxes World::getCollidingBoundingBoxes is gathering (Water).
extern Entity *g_collidingEntity;

inline bool on(int m) { return g_on[m]; }
inline bool isLocal(const Entity *e) { return e != nullptr && (e == g_local[0] || e == g_local[1]); }
inline bool onFor(int m, const Entity *e) { return g_on[m] && isLocal(e); }

float highJumpFactor();
bool fastBreakInstant();
bool noRenderItems();
// Build: replicate the block just placed against (x,y,z) side.
void onBlockPlaced(int x, int y, int z, int side);
// ESP / Tracers / ChestFinder / BlockESP lines, drawn inside the world pass.
void renderWorldOverlay(float partialTicks);
}
