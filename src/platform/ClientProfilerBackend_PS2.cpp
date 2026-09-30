#include "platform/WorkProfiler.h"
#include "platform/ExtendedProfiler.h"
#include "platform/ClientProfilerBackend.h"

#include "platform/Log.h"
#include "net/minecraft/src/RenderGlobal.h"
#include "net/minecraft/src/World.h"
#include "ps2/render/Ps2Tuning.h"
#include "ps2/diagnostics/Ps2OptimizationValidation.h"

#include <cstdio>
#include <cstring>

extern "C" void ps2_perf_frame_begin();
extern "C" void ps2_perf_frame_end();
extern "C" void ps2_perf_format_and_reset(char* out, int outSize);
extern "C" void ps2_perf_add_tick_ns(long long ns, int ticks);
extern "C" void ps2_perf_add_render_ns(long long ns);
extern "C" void ps2_perf_add_display_update_ns(long long ns);
extern "C" void ps2_perf_add_lighting_ns(long long ns);
extern "C" long ps2_dbg_ram_free_kb();
extern "C" long ps2_dbg_malloc_used_kb();
extern "C" long ps2_dbg_malloc_free_kb();
extern "C" void ps2_dbg_draw_dump();
extern "C" void ps2_dbg_depth_dump();
extern "C" long ps2_dbg_gs_queue_ram_bytes();
extern "C" void ps2_dbg_texture_ram_bytes(long* pixOut, long* clutOut, long* remapOut);
int ps2_mesh_staging_slots_in_use();

namespace
{
long textureRamKb()
{
	long pix = 0, clut = 0, remap = 0;
	ps2_dbg_texture_ram_bytes(&pix, &clut, &remap);
	return (pix + clut + remap) / 1024;
}

#if MC_LOG_LEVEL >= 2
// McLog::write formats through a 768-byte buffer (Log.cpp), and the perf line
// outgrew that when the render-phase breakdown was added: every sample lost
// everything from "mesh pass" onward, plus the whole chunk provider string,
// with no indication that it had happened -- vsnprintf truncates silently.
// Widening that buffer would charge every other log call site for this one
// line, so emit the line in pieces instead.
//
// Segments break on the " | " field separators the formatter already writes,
// so a break never lands inside a number.
void logFrameSegments(const char* prefix, const char* text)
{
	static const size_t kSegmentChars = 480;

	if (text == nullptr)
		return;

	const char* cursor = text;
	while (*cursor != '\0')
	{
		size_t take = std::strlen(cursor);
		if (take > kSegmentChars)
		{
			take = kSegmentChars;
			size_t split = take;
			while (split > 0 && std::strncmp(cursor + split, " | ", 3) != 0)
				--split;
			if (split > 0)
				take = split;
		}
		MC_LOG_DEBUG("frame", "%s %.*s\n", prefix, (int)take, cursor);
		cursor += take;
		while (*cursor == ' ' || *cursor == '|')
			++cursor;
	}
}
#endif
}

namespace ClientProfilerBackend
{
void frameBegin() { ps2_perf_frame_begin(); }
void ticks(long long ns, int ticksThisFrame) { ps2_perf_add_tick_ns(ns, ticksThisFrame); }
void lighting(long long ns) { ps2_perf_add_lighting_ns(ns); }
void displayUpdate(long long ns) { ps2_perf_add_display_update_ns(ns); }
void render(long long ns) { ps2_perf_add_render_ns(ns); }

void frameEnd(long long frameNs, long long tickNs, long long renderNs,
              int, int chunkUpdates, World* world, RenderGlobal* renderGlobal)
{
	ps2_perf_frame_end();
#ifdef PS2_OPTIMIZATION_VALIDATION
	if (world != nullptr)
		Ps2OptimizationValidation::frameEnd(frameNs);
#endif
#if MC_LOG_LEVEL >= 2
	static int frameCount = 0;
	++frameCount;
	if (world != nullptr && (frameCount <= 5 || (frameCount % 120) == 0))
	{
		// Sized for the whole line the formatter can now produce: the render
		// phase list, the tick phase list, the populate and streaming spans and
		// the mesh pass figures. logFrameSegments below is what keeps it inside
		// the log transport; this buffer only has to hold it first.
		char phases[2560] = {};
		ps2_perf_format_and_reset(phases, (int)sizeof(phases));
		const std::string chunks = world->getChunkProviderStats();
		RenderTerrainMeshRam meshRam;
		if (renderGlobal != nullptr)
			renderGlobal->terrainMeshRamBreakdown(meshRam);

		char framePrefix[24] = {};
		std::snprintf(framePrefix, sizeof(framePrefix), "frame=%d", frameCount);

		MC_LOG_DEBUG("frame", "%s tick=%ldms render=%ldms free=%ldKB mallocUsed=%ldKB mallocFree=%ldKB mesh=%ldKB(raw0=%ld raw1=%ld packed=%ld groups=%ld stg=%ld) tex=%ldKB gsq=%ldKB stage=%d/%d ents=%d lightQ=%d updates=%d pending=%d\n",
		             framePrefix,
		             (long)(tickNs / 1000000LL), (long)(renderNs / 1000000LL),
		             ps2_dbg_ram_free_kb(), ps2_dbg_malloc_used_kb(), ps2_dbg_malloc_free_kb(),
		             (long)(meshRam.total() / 1024),
		             (long)(meshRam.liveOpaque / 1024), (long)(meshRam.liveTranslucent / 1024),
		             (long)(meshRam.packedMesh / 1024), (long)(meshRam.faceGroups / 1024),
		             (long)(meshRam.stagingPool / 1024),
		             textureRamKb(), ps2_dbg_gs_queue_ram_bytes() / 1024,
		             ps2_mesh_staging_slots_in_use(), PS2_MESH_STAGING_SLOTS,
		             (int)world->getLoadedEntityList().size(),
		             (int)world->getPendingLightingUpdateCount(),
		             chunkUpdates,
		             renderGlobal != nullptr ? (int)renderGlobal->pendingRendererUpdateCount() : 0);
		logFrameSegments(framePrefix, phases);
		logFrameSegments(framePrefix, chunks.c_str());
		platformLogWorkProfileAndReset(frameCount);
#if MC_LOG_LEVEL > 2
		platformLogExtendedProfileAndReset(frameCount);
#endif
		ps2_dbg_draw_dump();
		ps2_dbg_depth_dump();
	}
#endif
}
}
