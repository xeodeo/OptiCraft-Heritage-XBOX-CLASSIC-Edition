#pragma once

// Central Xbox tuning knobs (64 MB unified memory budget).
//
// Deliberately small. The Xbox renders and simulates through the desktop code
// paths (display-list WorldRenderer, vanilla pathfinding and collision), so it
// takes the desktop defaults for everything except what the memory budget
// really requires: a bounded resident world, a short render distance and a
// mob cap. The Wii knobs this file was first copied from (vertical renderer
// window, incremental/deferred population, collision and AI shortcuts) belong
// to the Wii's native terrain pipeline and broke the desktop renderer here:
// only a centered band of sections was drawn and the rest of the terrain was
// missing.

#if PLATFORM_XBOX

#undef  PLATFORM_BOUNDED_WORLD
#define PLATFORM_BOUNDED_WORLD                   1

// Render distance: Short (2) by default to fit in 64 MB RAM.
#undef  PLATFORM_DEFAULT_RENDER_DISTANCE
#define PLATFORM_DEFAULT_RENDER_DISTANCE         2

#undef  PLATFORM_DEFAULT_MIPMAP_LEVEL
#define PLATFORM_DEFAULT_MIPMAP_LEVEL            0

// Preload radius (32 blocks = 5x5 chunk columns on startup).
#undef  PLATFORM_PRELOAD_RADIUS_BLOCKS
#define PLATFORM_PRELOAD_RADIUS_BLOCKS           32

// Resident chunk cache & unloads for bounded memory.
#undef  PLATFORM_PLAYER_UPDATE_CHUNK_RANGE_BLOCKS
#define PLATFORM_PLAYER_UPDATE_CHUNK_RANGE_BLOCKS 16
#undef  PLATFORM_CHUNK_CACHE_RADIUS
#define PLATFORM_CHUNK_CACHE_RADIUS              5
#undef  PLATFORM_CHUNK_UNLOAD_RADIUS
#define PLATFORM_CHUNK_UNLOAD_RADIUS             6
#undef  PLATFORM_MAX_CHUNK_UNLOADS_PER_TICK
#define PLATFORM_MAX_CHUNK_UNLOADS_PER_TICK      8
#undef  PLATFORM_MIN_UNUSED_TICKS_BEFORE_UNLOAD
#define PLATFORM_MIN_UNUSED_TICKS_BEFORE_UNLOAD  20

// Mob cap.
#undef  PLATFORM_MAX_LIVE_MOBS
#define PLATFORM_MAX_LIVE_MOBS                   40

// Chunk generation around the player (bounded-world streaming).
#undef  PLATFORM_GENERATE_SYNC_RADIUS
#define PLATFORM_GENERATE_SYNC_RADIUS            2
#undef  PLATFORM_GENERATE_CHUNKS_PER_TICK
#define PLATFORM_GENERATE_CHUNKS_PER_TICK        1

// Autosave rate.
#undef  PLATFORM_AUTOSAVE_PERIOD_TICKS
#define PLATFORM_AUTOSAVE_PERIOD_TICKS           1200
#undef  PLATFORM_INCREMENTAL_CHUNK_SAVE_LIMIT
#define PLATFORM_INCREMENTAL_CHUNK_SAVE_LIMIT    4

// UI scaling (640x480).
#define XBOX_LEGACY_GUI_SCALE                    2
#define XBOX_LEGACY_CREATE_WORLD_PANEL_WIDTH     310

#endif // PLATFORM_XBOX
