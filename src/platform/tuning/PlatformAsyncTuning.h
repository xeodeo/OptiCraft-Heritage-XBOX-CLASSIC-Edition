#pragma once

// -----------------------------------------------------------------------------
// Async chunk-generation scheduler
// -----------------------------------------------------------------------------
#if PLATFORM_ASYNC_CHUNK_GENERATION
#  if PLATFORM_PC_LEGACY
#    define PLATFORM_ASYNC_GENERATION_QUEUE_LIMIT        PC_LEGACY_ASYNC_GENERATION_QUEUE_LIMIT
#    define PLATFORM_ASYNC_GENERATION_REQUESTS_PER_TICK  PC_LEGACY_ASYNC_GENERATION_REQUESTS_PER_TICK
#    define PLATFORM_ASYNC_GENERATION_PUBLISH_PER_TICK   PC_LEGACY_ASYNC_GENERATION_PUBLISH_PER_TICK
#    define PLATFORM_ASYNC_GENERATION_THREAD_PRIORITY    PC_LEGACY_ASYNC_GENERATION_THREAD_PRIORITY
#    define PLATFORM_ASYNC_GENERATION_AFFINITY_MASK      PC_LEGACY_ASYNC_GENERATION_AFFINITY_MASK
#    define PLATFORM_ASYNC_ISOLATED_BIOME_SOURCE         PC_LEGACY_ASYNC_ISOLATED_BIOME_SOURCE
#    define PLATFORM_ASYNC_GENERATION_REQUESTS_PER_FRAME PC_LEGACY_ASYNC_GENERATION_REQUESTS_PER_FRAME
#    define PLATFORM_ASYNC_GENERATION_PUBLISH_PER_FRAME  PC_LEGACY_ASYNC_GENERATION_PUBLISH_PER_FRAME
// The desktop worker keeps Chunk construction on the game thread (see the
// scheduler construction in ChunkProvider.cpp), so saved chunks stay there too.
#    define PLATFORM_ASYNC_CHUNK_DECODE                  0
#    define PLATFORM_ASYNC_NEAREST_FIRST                 0
#  elif PLATFORM_WII
#    define PLATFORM_ASYNC_GENERATION_QUEUE_LIMIT        PLATFORM_WII_ASYNC_GENERATION_QUEUE_LIMIT
#    define PLATFORM_ASYNC_GENERATION_REQUESTS_PER_TICK  PLATFORM_WII_ASYNC_GENERATION_REQUESTS_PER_TICK
#    define PLATFORM_ASYNC_GENERATION_PUBLISH_PER_TICK   PLATFORM_WII_ASYNC_GENERATION_PUBLISH_PER_TICK
#    define PLATFORM_ASYNC_GENERATION_THREAD_PRIORITY    PLATFORM_WII_ASYNC_GENERATION_THREAD_PRIORITY
#    define PLATFORM_ASYNC_GENERATION_AFFINITY_MASK      0
// The worker samples biomes for terrain, caves and ravines through its own
// WorldChunkManager; the world's one has a BiomeCache the game thread mutates.
#    define PLATFORM_ASYNC_ISOLATED_BIOME_SOURCE         1
#    define PLATFORM_ASYNC_GENERATION_REQUESTS_PER_FRAME PLATFORM_WII_ASYNC_GENERATION_REQUESTS_PER_FRAME
#    define PLATFORM_ASYNC_GENERATION_PUBLISH_PER_FRAME  PLATFORM_WII_ASYNC_GENERATION_PUBLISH_PER_FRAME
// Decode saved chunks (NBT parse, block/light arrays, heightmap, skylight
// regeneration) on the worker, the same way it already constructs generated
// chunks; only entity construction is left for publish. Before this, a saved
// world paid the whole decode on the main thread per published column.
#    define PLATFORM_ASYNC_CHUNK_DECODE                  1
// The worker takes the queued column nearest the player's chunk instead of
// the oldest request. The queue is at most QUEUE_LIMIT entries, so the scan
// is a handful of compares per column.
#    define PLATFORM_ASYNC_NEAREST_FIRST                 1
#  else
#    define PLATFORM_ASYNC_GENERATION_QUEUE_LIMIT        0
#    define PLATFORM_ASYNC_GENERATION_REQUESTS_PER_TICK  0
#    define PLATFORM_ASYNC_GENERATION_PUBLISH_PER_TICK   0
#ifndef PLATFORM_ASYNC_GENERATION_THREAD_PRIORITY
#    define PLATFORM_ASYNC_GENERATION_THREAD_PRIORITY    64
#endif
#    define PLATFORM_ASYNC_GENERATION_AFFINITY_MASK      0
#    define PLATFORM_ASYNC_ISOLATED_BIOME_SOURCE         0
#    define PLATFORM_ASYNC_GENERATION_REQUESTS_PER_FRAME 0
#    define PLATFORM_ASYNC_GENERATION_PUBLISH_PER_FRAME  0
#    define PLATFORM_ASYNC_CHUNK_DECODE                  0
#    define PLATFORM_ASYNC_NEAREST_FIRST                 0
#  endif
#else
#  define PLATFORM_ASYNC_GENERATION_QUEUE_LIMIT        0
#  define PLATFORM_ASYNC_GENERATION_REQUESTS_PER_TICK  0
#  define PLATFORM_ASYNC_GENERATION_PUBLISH_PER_TICK   0
#ifndef PLATFORM_ASYNC_GENERATION_THREAD_PRIORITY
#  define PLATFORM_ASYNC_GENERATION_THREAD_PRIORITY    64
#endif
#  define PLATFORM_ASYNC_GENERATION_AFFINITY_MASK      0
#  define PLATFORM_ASYNC_ISOLATED_BIOME_SOURCE         0
#  define PLATFORM_ASYNC_GENERATION_REQUESTS_PER_FRAME 0
#  define PLATFORM_ASYNC_GENERATION_PUBLISH_PER_FRAME  0
#  define PLATFORM_ASYNC_CHUNK_DECODE                  0
#  define PLATFORM_ASYNC_NEAREST_FIRST                 0
#endif
