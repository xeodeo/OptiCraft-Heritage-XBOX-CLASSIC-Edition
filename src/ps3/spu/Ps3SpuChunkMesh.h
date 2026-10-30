#pragma once

// Section meshing on the SPUs (PLATFORM_SPU_CHUNK_MESH, PS3 only), PPU side.
// SPU side: src/ps3/spu/tasks/ChunkMesh (4J's ChunkUpdate idea, our renderer).
//
// Used by the mesh worker thread (src/ps3/render/Ps3MeshWorker.cpp) for each
// section's opaque pass, with one ticket per worker job slot:
//
//   if (ps3SpuMeshBegin(slot, cache))         // builds the SPU input, starts the job
//       ... PPU renders every block with !ps3SpuMeshHandles(slot, id) ...
//   ps3SpuMeshEnd(slot, cache, tessellator, x0, y0, z0);
//                                             // waits, appends the SPU's quads
//
// End always leaves the section complete: if the SPU overflowed, failed or the
// verify mode found a difference, the PPU renders the SPU's blocks right there.
// The SPU's vertices are the Tessellator's own; the first
// PS3_SPU_MESH_VERIFY_SECTIONS sections are also meshed on the PPU and compared
// in full: position, UV and brightness exactly, colour bytes within +-1. Any
// difference out of that tolerance keeps meshing on the PPU for the session.

#include "platform/PlatformConfig.h"

#if PLATFORM_SPU_CHUNK_MESH

#include "java/Type.h"

class PcLegacySectionCache;
class Tessellator;

constexpr int kPs3SpuMeshTickets = 8;

// Worker thread. True when the SPU took the section's SPU-meshable blocks.
bool ps3SpuMeshBegin(int slot, PcLegacySectionCache &cache);
// Worker thread, between Begin and End: whether the SPU renders this block id.
bool ps3SpuMeshHandles(int slot, int_t blockId);
// Worker thread. Completes a begun section into tessellator (inside its open
// pass-0 batch, translation set to minus the section origin).
void ps3SpuMeshEnd(int slot, PcLegacySectionCache &cache, Tessellator &tessellator,
    int_t x0, int_t y0, int_t z0);
// Worker thread: waits for a begun section's SPU job and drops its result
// (the section was abandoned, e.g. a block renderer threw).
void ps3SpuMeshCancel(int slot);
// Worker thread: blocks of pass 0 the PPU rendered itself (for the log).
void ps3SpuMeshNotePpuBlocks(int blocks);
// Main thread, every few seconds: the "ps3.spumesh" line.
void ps3SpuMeshLogCounters();

#endif
