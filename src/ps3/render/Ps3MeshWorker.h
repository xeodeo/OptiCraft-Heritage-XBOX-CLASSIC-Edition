#pragma once

// Chunk meshing on a worker thread (PLATFORM_THREADED_TERRAIN_MESH).
//
// The Legacy Console Edition meshed sections on the PPU's second hardware
// thread; this is the same split for the desktop-derived builder:
//
//   main thread   WorldRenderer::pcLegacyThreadedBuildStep picks a section the
//                 scheduler chose, takes a full snapshot of its 18^3 block
//                 neighbourhood (ids, metadata, light, biomes) and collects its
//                 tile entities, and queues it here ("meshSnapshot" stage).
//   worker        meshes the snapshot with its own Tessellator into CPU vertex
//                 arrays. It reads nothing but the snapshot and the immutable
//                 block/texture tables: no World, Chunk, ChunkCache or RSX.
//   main thread   a later step of the same renderer takes the finished job and
//                 records the display lists (pcLegacyPublishBuild,
//                 "meshPublish" stage). RSX memory is only touched here.
//
// A job is owned by one WorldRenderer. Whatever invalidates a renderer's build
// (moving it, dropping it, destroying it) releases its job: a job the worker is
// still meshing is orphaned and its result thrown away when it finishes. The
// main thread never waits for the worker, except at shutdown.

#include "platform/PlatformConfig.h"

#if PLATFORM_THREADED_TERRAIN_MESH

#include <cstdint>

class WorldRenderer;
struct PcLegacyTerrainStaging;

// True while the worker thread runs. When it is false (not started yet, failed
// to start, shut down) the synchronous incremental builder is used.
bool ps3MeshWorkerRunning();

// Main thread, once per scheduler call: starts the worker the first time,
// reclaims the jobs of released renderers and logs the counters.
void ps3MeshWorkerPump();

// Main thread. A free job reserved for owner, or null when every job is in
// flight. The caller fills it (sectionCache detached, origin set) and submits it.
PcLegacyTerrainStaging *ps3MeshWorkerAcquire(WorldRenderer *owner);
void ps3MeshWorkerSubmit(PcLegacyTerrainStaging *job);

// Main thread. Whether owner has a job queued, in work or finished.
bool ps3MeshWorkerOwnsJob(const WorldRenderer *owner);
// Main thread. owner's finished job, or null while it is still queued or in
// work (or owner has none). The job stays owned until released.
PcLegacyTerrainStaging *ps3MeshWorkerFinished(WorldRenderer *owner);
// Main thread. Drops owner's job, finished or not.
void ps3MeshWorkerRelease(const WorldRenderer *owner);

// Main thread, before a tile entity is deleted: scrubs it from the tile entity
// lists of jobs not yet published (they are published frames later).
void ps3MeshWorkerForgetTileEntity(const void *tileEntity);

// Main-thread cost accounting for the periodic log line.
void ps3MeshWorkerNoteSnapshot(std::uint64_t micros);
// Parts of a snapshot: ChunkCache, block ids (section cache), tile entity
// scan, metadata, light and biomes.
void ps3MeshWorkerNoteSnapshotParts(std::uint64_t chunkCacheUs, std::uint64_t idsUs, std::uint64_t tileEntitiesUs,
    std::uint64_t metaUs, std::uint64_t lightUs, std::uint64_t biomeUs);
void ps3MeshWorkerNotePublish(std::uint64_t micros);

// Stops and joins the worker (game shutdown). Bounded: gives up on the join
// after a short wait rather than hold the exit.
void ps3MeshWorkerShutdown();

#endif
