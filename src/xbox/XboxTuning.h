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
// Each unload saves the column synchronously (NBT build + write). A row of
// columns leaving the radius together cost 60-110 ms in one tick at 8; two
// per tick spreads them out. Memory pressure still unloads faster through
// PLATFORM_EMERGENCY_CHUNK_UNLOADS_PER_TICK.
#undef  PLATFORM_MAX_CHUNK_UNLOADS_PER_TICK
#define PLATFORM_MAX_CHUNK_UNLOADS_PER_TICK      2
#undef  PLATFORM_MIN_UNUSED_TICKS_BEFORE_UNLOAD
#define PLATFORM_MIN_UNUSED_TICKS_BEFORE_UNLOAD  20

// Bounded, reused pathfinder (PlatformConfig.h), with the low-end PC limits:
// the desktop defaults are "unlimited", which the bounded pathfinder turns
// into an impossible preallocation (std::length_error at the first mob).
#undef  PLATFORM_PATHFIND_MAX_NODES
#define PLATFORM_PATHFIND_MAX_NODES              300
#undef  PLATFORM_PATHFIND_BUDGET_PER_TICK
#define PLATFORM_PATHFIND_BUDGET_PER_TICK        3
#undef  PLATFORM_REUSE_PATHFINDER
#define PLATFORM_REUSE_PATHFINDER                1
// Incremental terrain builder (PLATFORM_INCREMENTAL_TERRAIN_BUILD): one
// section is built in slices of 2.5 ms, at most 5 ms of meshing per frame,
// with the low-end PC scheduler. Same geometry as the vanilla builder; it only
// spreads the work so a heavy section no longer costs a whole frame.
#undef  PLATFORM_MESH_BUDGET
#define PLATFORM_MESH_BUDGET                     PC_LEGACY_MESH_BUDGET
#undef  PLATFORM_MIN_RENDERER_UPDATES_PER_FRAME
#define PLATFORM_MIN_RENDERER_UPDATES_PER_FRAME  PC_LEGACY_MIN_RENDERER_UPDATES_PER_FRAME
#undef  PLATFORM_MAX_RENDERER_UPDATES_PER_FRAME
#define PLATFORM_MAX_RENDERER_UPDATES_PER_FRAME  PC_LEGACY_MAX_RENDERER_UPDATES_PER_FRAME
#undef  PLATFORM_RENDERER_UPDATE_CANDIDATES_PER_FRAME
#define PLATFORM_RENDERER_UPDATE_CANDIDATES_PER_FRAME PC_LEGACY_RENDERER_UPDATE_CANDIDATES_PER_FRAME
#undef  PLATFORM_COALESCE_MESH_REBUILDS
#define PLATFORM_COALESCE_MESH_REBUILDS          PC_LEGACY_COALESCE_MESH_REBUILDS
#undef  PLATFORM_CHUNK_BUILD_BLOCKS_PER_STEP
#define PLATFORM_CHUNK_BUILD_BLOCKS_PER_STEP     PC_LEGACY_CHUNK_BUILD_BLOCKS_PER_STEP
#undef  PLATFORM_CHUNK_BUILD_BUDGET_MS
#define PLATFORM_CHUNK_BUILD_BUDGET_MS           PC_LEGACY_CHUNK_BUILD_BUDGET_MS
#undef  PLATFORM_CHUNK_BUILD_STEP_US
#define PLATFORM_CHUNK_BUILD_STEP_US             PC_LEGACY_CHUNK_BUILD_STEP_US
// The low-end PC's direct cube emitter writes triangles (6 vertices a face);
// the tessellator path below keeps quads (4), which the NV2A draws natively.
#undef  PLATFORM_COMPACT_TERRAIN_VERTICES
#define PLATFORM_COMPACT_TERRAIN_VERTICES        0

// Tessellator: quads straight to D3DPT_QUADLIST (no conversion to triangles,
// a third fewer vertices to build, store and draw) and the console-sized
// staging buffer (it grows on demand; the desktop 8 MB one is never needed).
#undef  PLATFORM_TESSELLATOR_CONVERT_QUADS
#define PLATFORM_TESSELLATOR_CONVERT_QUADS       0
#undef  PLATFORM_TESSELLATOR_BUFFER_INTS
#define PLATFORM_TESSELLATOR_BUFFER_INTS         0x10000

// Behaviour-exact CPU shortcuts shared with the PS2 and low-end PC profiles:
// direct section reads for block collision, early outs where only "any
// collision" matters, cached entity queries and chunk-existence lookups.
#undef  PLATFORM_FAST_BLOCK_COLLISIONS
#define PLATFORM_FAST_BLOCK_COLLISIONS           1
#undef  PLATFORM_EARLY_UNIT_CUBE_COLLISION_TEST
#define PLATFORM_EARLY_UNIT_CUBE_COLLISION_TEST  1
#undef  PLATFORM_EARLY_COLLISION_EXIT
#define PLATFORM_EARLY_COLLISION_EXIT            1
#undef  PLATFORM_ENTITY_QUERY_CACHE
#define PLATFORM_ENTITY_QUERY_CACHE              1
#undef  PLATFORM_CACHE_ENTITY_CHUNK_EXISTENCE
#define PLATFORM_CACHE_ENTITY_CHUNK_EXISTENCE    1
#undef  PLATFORM_CACHE_RANDOM_TICK_CHUNKS
#define PLATFORM_CACHE_RANDOM_TICK_CHUNKS        1
#undef  PLATFORM_CACHE_SPAWN_CHUNKS
#define PLATFORM_CACHE_SPAWN_CHUNKS              1
#undef  PLATFORM_CACHE_RANDOM_DISPLAY_CHUNKS
#define PLATFORM_CACHE_RANDOM_DISPLAY_CHUNKS     1
#undef  PLATFORM_FAST_CHUNK_BLOCK_READS
#define PLATFORM_FAST_CHUNK_BLOCK_READS          1

// New columns: build the section arrays in bulk and mark the renderers once
// per column instead of ~7000 single-block render updates from the initial
// skylight pass. The world data is identical (PS2 / low-end PC knobs).
#undef  PLATFORM_BATCH_INITIAL_SKYLIGHT_RENDER_UPDATES
#define PLATFORM_BATCH_INITIAL_SKYLIGHT_RENDER_UPDATES 1
#undef  PLATFORM_BULK_GENERATED_CHUNK_IMPORT
#define PLATFORM_BULK_GENERATED_CHUNK_IMPORT     1

// Mob cap.
#undef  PLATFORM_MAX_LIVE_MOBS
#define PLATFORM_MAX_LIVE_MOBS                   40

// Chunk generation around the player (bounded-world streaming).
// Only the 3x3 columns around the player generate on demand; the rest go
// through the incremental generator below, never inside one frame.
#undef  PLATFORM_GENERATE_SYNC_RADIUS
#define PLATFORM_GENERATE_SYNC_RADIUS            1
#undef  PLATFORM_GENERATE_CHUNKS_PER_TICK
#define PLATFORM_GENERATE_CHUNKS_PER_TICK        1

// Incremental chunk generation (the PS2 generator): a new column is built in
// steps -- noise, surface, each slice of the cave/ravine/structure sweeps --
// under a per-tick and a per-frame time budget, instead of 25-40 ms in one
// frame. Same terrain, same seed results; only the work is spread out.
#undef  PLATFORM_INCREMENTAL_CHUNK_GENERATION
#define PLATFORM_INCREMENTAL_CHUNK_GENERATION    1
#undef  PLATFORM_GENERATION_STEPS_PER_TICK
#define PLATFORM_GENERATION_STEPS_PER_TICK       16
#undef  PLATFORM_GENERATION_BUDGET_US
#define PLATFORM_GENERATION_BUDGET_US            4000
#undef  PLATFORM_GENERATION_STEPS_PER_FRAME
#define PLATFORM_GENERATION_STEPS_PER_FRAME      8
#undef  PLATFORM_GENERATION_FRAME_BUDGET_US
#define PLATFORM_GENERATION_FRAME_BUDGET_US      3000
#undef  PLATFORM_GENERATION_SOURCE_COLUMNS_PER_STEP
#define PLATFORM_GENERATION_SOURCE_COLUMNS_PER_STEP 16
#undef  PLATFORM_STRUCTURE_SOURCE_COLUMNS_PER_STEP
#define PLATFORM_STRUCTURE_SOURCE_COLUMNS_PER_STEP  289

// Autosave rate.
#undef  PLATFORM_AUTOSAVE_PERIOD_TICKS
#define PLATFORM_AUTOSAVE_PERIOD_TICKS           1200
#undef  PLATFORM_INCREMENTAL_CHUNK_SAVE_LIMIT
#define PLATFORM_INCREMENTAL_CHUNK_SAVE_LIMIT    4

// UI scaling (640x480).
#define XBOX_LEGACY_GUI_SCALE                    2
#define XBOX_LEGACY_CREATE_WORLD_PANEL_WIDTH     310

#endif // PLATFORM_XBOX
