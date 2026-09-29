#include "ps2/render/Ps2TerrainSectionDraw.h"
#ifdef PS2_PLATFORM

#include <algorithm>
#include <cstdint>
#include <vector>

#include "platform/PlatformTuning.h"
#include "ps2/render/Ps2CaptureLayout.h"
#include "ps2/render/Ps2TerrainCulling.h"
#include "ps2/render/Ps2TerrainCommands.h"
#include "ps2/render/Ps2TerrainMesh.h"
#include "ps2/render/Ps2TerrainMeshView.h"
#include "ps2/render/Ps2TerrainRuntime.h"
#include "ps2/render/Ps2Vu1Terrain.h"
#include "ps2/diagnostics/Ps2OptimizationValidation.h"

#ifdef PS2_RENDER_STATS
#define PS2_TERRAIN_CLUSTER_STAT(expr) do { expr; } while (0)
#else
#define PS2_TERRAIN_CLUSTER_STAT(expr) do { } while (0)
#endif

Ps2TerrainDrawResult ps2_terrain_draw_section(const Ps2RendererFrame& frame,
                                              const Ps2TerrainSectionView& section,
                                              const Ps2TerrainFallbackDraw& fallback)
{
    Ps2TerrainRuntimeState& runtime = ps2_terrain_runtime();
    Ps2TerrainClusterStats& s_clusterStats = runtime.clusterStats;
    Ps2TerrainPass& s_currentPass = runtime.currentPass;
    Ps2OpaqueSubmitPhase& s_opaqueSubmitPhase = runtime.opaqueSubmitPhase;
    int& s_queuedTerrainCount = runtime.queuedCount;
    Ps2QueuedTerrainSection* const s_queuedTerrain = runtime.queued;
    bool& s_currentVu1Retry = runtime.currentVu1Retry;
    bool& s_forceVu0All = runtime.forceVu0All;
    Ps2ClusterClassification*& s_classification = runtime.classification;
    constexpr int kMaxQueuedTerrainSections = PS2_MAX_QUEUED_TERRAIN_SECTIONS;

    Ps2TerrainDrawResult result = { 0, 0, true };
	if (s_currentPass == PS2_TERRAIN_PASS_OPAQUE &&
		s_opaqueSubmitPhase == PS2_OPAQUE_SUBMIT_QUEUE)
	{
		if (s_queuedTerrainCount >= kMaxQueuedTerrainSections)
		{
			result.complete = false;
			return result;
		}

		const int queuedIndex = s_queuedTerrainCount++;
		Ps2QueuedTerrainSection& queued = s_queuedTerrain[queuedIndex];
		queued.frame = frame;
		queued.section = section;
		queued.fallback = fallback;
		queued.forceVu0All = false;
		queued.commandReady = false;
		queued.commandFailed = false;
		queued.commandReady = ps2_terrain_build_section_commands(queuedIndex);
		return result;
	}

	if (section.vertexCount <= 0)
        return result;

    const std::size_t slotsPerVertex = Ps2CaptureLayout::Slots;
	const bool packedOpaque = s_currentPass == PS2_TERRAIN_PASS_OPAQUE &&
		section.opaqueMesh != nullptr && section.opaqueMesh->valid() &&
		section.opaqueMesh->vertexCount() >= section.vertexCount;
	if (!packedOpaque && (section.raw == nullptr || section.rawIntCount == 0 ||
		(section.rawIntCount % slotsPerVertex) != 0u))
    {
        result.complete = false;
        return result;
    }

	const int_t totalVertices = packedOpaque
		? section.vertexCount
		: (int_t)std::min((std::size_t)section.vertexCount,
			section.rawIntCount / slotsPerVertex);
    if (totalVertices <= 0)
        return result;

    Ps2NativeDrawContext nativeContext;
    bool nativePathUsable = false;
    if (section.nativeEnabled)
    {
        nativePathUsable = ps2_renderer_prepare_terrain_context(
            nativeContext, frame,
            section.translateX, section.translateY, section.translateZ,
            section.fullyInside);
    }

	const bool directVu1Usable = PS2_DIRECT_VU1_TERRAIN && !s_forceVu0All &&
        s_currentPass == PS2_TERRAIN_PASS_OPAQUE && nativePathUsable &&
		packedOpaque && ps2_vu1_terrain_pass_ready() &&
        section.drawMode == PS2_NATIVE_PRIM_QUADS && section.hasTexture &&
        !section.hasNormals;

    bool abortDraw = false;
    auto emitVu0Raw = [&](const int_t* raw, int_t vertexCount, bool fullyInside)
    {
		if (s_opaqueSubmitPhase == PS2_OPAQUE_SUBMIT_VU1)
			return;
        if (abortDraw || raw == nullptr || vertexCount <= 0)
        {
            if (raw == nullptr || vertexCount < 0)
                result.complete = false;
            return;
        }

#if defined(PS2_ENABLE_VU1_TERRAIN)
        // Any native VU0/gsKit draw must first submit and drain queued Path1
        // work from preceding direct terrain ranges.
        ps2_render_release_path1();
#endif

        int_t emitted = 0;
        while (emitted < vertexCount)
        {
            const int_t batchVertices = ps2_terrain_bounded_batch_size(vertexCount - emitted, section.drawMode);
            if (batchVertices <= 0)
            {
                result.complete = false;
                abortDraw = true;
                return;
            }

            const int_t* batchRaw = raw + (std::size_t)emitted * slotsPerVertex;

            if (nativePathUsable)
            {
                const Ps2NativeMeshView mesh = ps2_terrain_make_raw_mesh(section, batchRaw, batchVertices);
                Ps2NativeDrawContext rangeContext = nativeContext;
                rangeContext.fullyInside = fullyInside;
                if (ps2_renderer_draw_prepared(mesh, rangeContext))
                {
#ifdef PS2_OPTIMIZATION_VALIDATION
                    Ps2OptimizationValidation::terrainVu0Submit(batchVertices);
#endif
                    result.nativeVertices += batchVertices;
                    emitted += batchVertices;
                    continue;
                }

                nativePathUsable = false;
            }

            if (fallback.draw == nullptr)
            {
                result.complete = false;
                abortDraw = true;
                return;
            }

            fallback.draw(fallback.user, batchRaw, batchVertices,
                          section.drawMode,
                          section.hasTexture, section.hasColor, section.hasNormals);
            result.fallbackVertices += batchVertices;
            emitted += batchVertices;
        }
    };

	auto emitVu0Packed = [&](const short* positions,
								 const short* texCoords,
								 const unsigned char* colors,
								 int_t firstVertex,
								 int_t vertexCount,
								 bool fullyInside,
                                 const Ps2NativeClampRun* clampRuns,
                                 int clampRunCount)
	{
		if (s_opaqueSubmitPhase == PS2_OPAQUE_SUBMIT_VU1)
			return;
		if (abortDraw || positions == nullptr || texCoords == nullptr || colors == nullptr ||
			firstVertex < 0 || vertexCount <= 0)
		{
			result.complete = false;
			abortDraw = true;
			return;
		}

#if defined(PS2_ENABLE_VU1_TERRAIN)
		ps2_render_release_path1();
#endif

		int_t emitted = 0;
		while (emitted < vertexCount)
		{
			const int_t batchVertices = ps2_terrain_bounded_batch_size(
				vertexCount - emitted, PS2_NATIVE_PRIM_QUADS);
			if (batchVertices <= 0 || !nativePathUsable)
			{
				result.complete = false;
				abortDraw = true;
				return;
			}

			const Ps2NativeMeshView mesh = ps2_terrain_make_packed_mesh(
				positions, texCoords, colors, firstVertex + emitted, batchVertices,
                clampRuns, clampRunCount);
			Ps2NativeDrawContext rangeContext = nativeContext;
			rangeContext.fullyInside = fullyInside;
			if (!ps2_renderer_draw_prepared(mesh, rangeContext))
			{
				nativePathUsable = false;
				result.complete = false;
				abortDraw = true;
				return;
			}

#ifdef PS2_OPTIMIZATION_VALIDATION
			Ps2OptimizationValidation::terrainVu0Submit(batchVertices);
#endif
			result.nativeVertices += batchVertices;
			emitted += batchVertices;
		}
	};

	// Zero-copy counterpart to emitVu0Packed: draws several non-contiguous
	// slices of the SAME mesh arrays in one native draw call instead of one
	// call per contiguous range. positions/texCoords/colors and clampRuns are
	// the mesh's own, un-gathered arrays -- no EE-side memcpy, matching the
	// direct VU1 terrain path's DMA REF scatter-gather (Ps2Vu1TerrainPackets.cpp).
	auto emitVu0PackedSliced = [&](const short* positions,
	                                const short* texCoords,
	                                const unsigned char* colors,
	                                const Ps2NativeSlice* slices,
	                                int sliceCount,
	                                int_t totalVertexCount,
	                                bool fullyInside,
	                                const Ps2NativeClampRun* clampRuns,
	                                int clampRunCount)
	{
		if (s_opaqueSubmitPhase == PS2_OPAQUE_SUBMIT_VU1)
			return;
		if (abortDraw || positions == nullptr || texCoords == nullptr || colors == nullptr ||
			slices == nullptr || sliceCount <= 0 || totalVertexCount <= 0)
		{
			result.complete = false;
			abortDraw = true;
			return;
		}

#if defined(PS2_ENABLE_VU1_TERRAIN)
		ps2_render_release_path1();
#endif

		if (!nativePathUsable)
		{
			result.complete = false;
			abortDraw = true;
			return;
		}

		const Ps2NativeMeshView mesh = ps2_terrain_make_packed_mesh(
			positions, texCoords, colors, 0, totalVertexCount,
			clampRuns, clampRunCount, slices, sliceCount);
		Ps2NativeDrawContext rangeContext = nativeContext;
		rangeContext.fullyInside = fullyInside;
		if (!ps2_renderer_draw_prepared(mesh, rangeContext))
		{
			nativePathUsable = false;
			result.complete = false;
			abortDraw = true;
			return;
		}

#ifdef PS2_OPTIMIZATION_VALIDATION
		Ps2OptimizationValidation::terrainVu0Submit(totalVertexCount);
#endif
		result.nativeVertices += totalVertexCount;
	};

    auto emitRange = [&](int_t firstVertex, int_t vertexCount, bool fullyInside)
    {
        if (abortDraw || firstVertex < 0 || vertexCount <= 0 ||
            firstVertex + vertexCount > totalVertices)
        {
            if (firstVertex < 0 || vertexCount < 0 || firstVertex + vertexCount > totalVertices)
                result.complete = false;
            return;
        }

#if defined(PS2_ENABLE_VU1_TERRAIN)
		if (s_opaqueSubmitPhase != PS2_OPAQUE_SUBMIT_VU0 &&
			nativePathUsable && directVu1Usable &&
            (fullyInside || PS2_VU1_CLIPPED_PARTIALS) &&
            (section.faceGroups == nullptr || section.faceGroups->valid) &&
            section.drawMode == PS2_NATIVE_PRIM_QUADS && section.hasTexture &&
            !section.hasNormals)
        {
            const Ps2Vu1TerrainDrawResult vu1 = ps2_vu1_terrain_draw_range(
				*section.opaqueMesh, firstVertex, vertexCount, frame.native,
                section.translateX, section.translateY, section.translateZ,
                fullyInside);
            if (vu1.status == PS2_VU1_TERRAIN_SUBMITTED)
            {
                PS2_TERRAIN_CLUSTER_STAT(++s_clusterStats.vu1Ranges);
                PS2_TERRAIN_CLUSTER_STAT(s_clusterStats.vu1Vertices += vu1.vertices);
#ifdef PS2_OPTIMIZATION_VALIDATION
                Ps2OptimizationValidation::terrainVu1Submit(vu1.vertices);
#endif
                result.nativeVertices += vu1.vertices;
                return;
            }
#ifdef PS2_OPTIMIZATION_VALIDATION
            Ps2OptimizationValidation::terrainVu1Retry(vu1.status == PS2_VU1_TERRAIN_FATAL);
#endif
            if (vu1.status == PS2_VU1_TERRAIN_FATAL)
            {
                result.nativeVertices += vu1.vertices;
                result.complete = false;
				s_currentVu1Retry = true;
                abortDraw = true;
                return;
            }

            // RETRY_NATIVE means no geometry from this range was accepted by
            // VU1. Queued submission records a whole-section VU0 replay;
            // immediate submission releases Path1 before writing Path3.
			s_currentVu1Retry = true;
			if (s_opaqueSubmitPhase == PS2_OPAQUE_SUBMIT_VU1)
			{
				// The VU0 phase will replay this section in full. Stop feeding
				// Path1 as soon as the section has requested that fallback.
				abortDraw = true;
				return;
			}
			ps2_render_release_path1();
        }
#endif
		if (s_opaqueSubmitPhase == PS2_OPAQUE_SUBMIT_VU1)
			return;

		if (packedOpaque)
		{
			const std::vector<Ps2TerrainTileRun>& clampRuns = section.opaqueMesh->runs();
			emitVu0Packed(section.opaqueMesh->positions(),
				section.opaqueMesh->texCoords(), section.opaqueMesh->colors(),
				firstVertex, vertexCount, fullyInside,
                clampRuns.empty() ? nullptr : &clampRuns[0], (int)clampRuns.size());
		}
		else
		{
			const int_t* rangeRaw = section.raw +
				(std::size_t)firstVertex * slotsPerVertex;
			emitVu0Raw(rangeRaw, vertexCount, fullyInside);
		}
    };

    auto emitClippedProbe = [&](int_t firstVertex, int_t vertexCount)
    {
#if defined(PS2_ENABLE_VU1_TERRAIN) && \
    PS2_VU1_CLIPPED_PROBE_BATCHES_PER_FRAME > 0 && !PS2_VU1_CLIPPED_PARTIALS
        if (abortDraw || s_opaqueSubmitPhase != PS2_OPAQUE_SUBMIT_VU1 ||
            !nativePathUsable || !directVu1Usable ||
            !ps2_vu1_terrain_clipped_probe_available())
            return;

        // This is deliberately a shadow submission. The authoritative VU0
        // gather below still includes the complete range, so an empty or
        // malformed clipped result cannot remove visible terrain.
        (void)ps2_vu1_terrain_probe_clipped_range(
            *section.opaqueMesh, firstVertex, vertexCount, frame.native,
            section.translateX, section.translateY, section.translateZ);
#else
        (void)firstVertex;
        (void)vertexCount;
#endif
    };

    if (section.faceGroups != nullptr && section.faceGroups->valid &&
        !section.faceGroups->ranges.empty())
    {
        const float eye[3] = {
            section.eyeLocalX,
            section.eyeLocalY,
            section.eyeLocalZ
        };

        // Second pass over a queued section: the MVP and the cluster bounds are
        // unchanged, so both classifications below would reproduce exactly what
        // the first pass already stored. See Ps2ClusterClassification.
        const bool reuseClassification =
            s_classification != nullptr && s_classification->valid;

        int clusterClass[PS2_MESH_CLUSTER_COUNT];
        int clusterGuardRisk[PS2_MESH_CLUSTER_COUNT];
        if (reuseClassification)
        {
            for (int cluster = 0; cluster < PS2_MESH_CLUSTER_COUNT; ++cluster)
            {
                clusterClass[cluster] = (int)s_classification->visibility[cluster];
                clusterGuardRisk[cluster] = (int)s_classification->guardRisk[cluster];
            }
        }
        else
        {
            Ps2TerrainCullingContext cullingContext = {};
            const Ps2TerrainCullingContext* culling = nullptr;
            if (nativePathUsable)
            {
                ps2_terrain_build_culling_context(cullingContext, nativeContext.mvp,
                                                  frame.native.viewW, frame.native.viewH);
                culling = &cullingContext;
            }
            ps2_terrain_classify_clusters(section.faceGroups->clusters,
                                          section.fullyInside, nativePathUsable, culling,
                                          clusterClass, clusterGuardRisk);
        }

        if (s_classification != nullptr && !reuseClassification)
        {
            for (int cluster = 0; cluster < PS2_MESH_CLUSTER_COUNT; ++cluster)
            {
                s_classification->visibility[cluster] = (unsigned char)clusterClass[cluster];
                s_classification->guardRisk[cluster] = (unsigned char)clusterGuardRisk[cluster];
            }
            s_classification->valid = true;
        }

        int_t clusterVertices[PS2_MESH_CLUSTER_COUNT] = {};
        const std::vector<Ps2MeshRange>& classifiedRanges = section.faceGroups->ranges;
        for (std::size_t i = 0; i < classifiedRanges.size(); ++i)
            clusterVertices[classifiedRanges[i].cluster()] += classifiedRanges[i].vertexCount();

		if (s_opaqueSubmitPhase != PS2_OPAQUE_SUBMIT_VU0)
		{
			PS2_TERRAIN_CLUSTER_STAT(++s_clusterStats.sections);
			for (int cluster = 0; cluster < PS2_MESH_CLUSTER_COUNT; ++cluster)
			{
				if (clusterVertices[cluster] <= 0)
					continue;
				if (clusterClass[cluster] == 2)
					PS2_TERRAIN_CLUSTER_STAT(++s_clusterStats.insideClusters);
				else if (clusterClass[cluster] == 1)
					PS2_TERRAIN_CLUSTER_STAT(++s_clusterStats.partialClusters);
				else
				{
					PS2_TERRAIN_CLUSTER_STAT(++s_clusterStats.outsideClusters);
					PS2_TERRAIN_CLUSTER_STAT(
						s_clusterStats.outsideVertices += clusterVertices[cluster]);
					continue;
				}

				const int risk = clusterGuardRisk[cluster];
				if (risk == PS2_CLUSTER_GUARD_SAFE && clusterClass[cluster] == 1)
				{
					PS2_TERRAIN_CLUSTER_STAT(++s_clusterStats.guardSafePartialClusters);
					PS2_TERRAIN_CLUSTER_STAT(
						s_clusterStats.guardSafePartialVertices += clusterVertices[cluster]);
				}
				if ((risk & PS2_CLUSTER_GUARD_NEAR) != 0)
					PS2_TERRAIN_CLUSTER_STAT(++s_clusterStats.guardNearRiskClusters);
				if ((risk & PS2_CLUSTER_GUARD_SIDE) != 0)
					PS2_TERRAIN_CLUSTER_STAT(++s_clusterStats.guardSideRiskClusters);
			}
		}

        auto faceVisible = [&](int group)
        {
            bool keep = true;
#if PLATFORM_FACE_BUCKET_CULL
            if (group != PS2_FACE_OTHER)
            {
                const int axis = group >> 1;
                const bool positive = (group & 1) == 0;
                const float margin = (float)PLATFORM_FACE_CULL_EYE_MARGIN;
                keep = positive
                    ? eye[axis] + margin > section.faceGroups->planeMin[group]
                    : eye[axis] - margin < section.faceGroups->planeMax[group];
            }
#else
            (void)group;
#endif
            return keep;
        };

        // Opaque geometry has no ordering requirement. Submit all direct VU1
        // ranges first, then gather the intersecting ranges into one tile-ordered
        // VU0 stream. This avoids both Path1/Path3 alternation and one tiny VU0
        // draw per cluster/face range.
        auto emitVu1Ranges = [&]()
        {
#if defined(PS2_ENABLE_VU1_TERRAIN) && !PS2_VU1_CLIPPED_PARTIALS
            // The packed mesh is tile-major. Face/cluster culling can punch small
            // holes inside one tile run; feeding every surviving fragment through
            // draw_range() made each hole become another MSCAL/XGKICK. Scatter the
            // surviving source slices into one VU input buffer with VIF UNPACKs and
            // kick once for up to 80 vertices. No EE vertex copy is required.
            if (packedOpaque && directVu1Usable && section.opaqueMesh != nullptr)
            {
                const std::vector<Ps2MeshRange>& ranges = section.faceGroups->ranges;
                const std::vector<Ps2TerrainTileRun>& tileRuns = section.opaqueMesh->runs();
                std::size_t rangeIndex = 0;

                for (std::size_t tr = 0; tr < tileRuns.size() && !abortDraw; ++tr)
                {
                    const Ps2TerrainTileRun& tile = tileRuns[tr];
                    const int_t tileBegin = (int_t)tile.firstVertex;
                    const int_t tileEnd = tileBegin + (int_t)tile.vertexCount;
                    while (rangeIndex < ranges.size() &&
                           ranges[rangeIndex].firstVertex + ranges[rangeIndex].vertexCount() <= tileBegin)
                        ++rangeIndex;

                    Ps2Vu1TerrainSlice slices[8];
                    int sliceCount = 0;
                    int batchVertices = 0;

                    auto flushSlices = [&]() -> bool
                    {
                        if (batchVertices <= 0)
                            return true;
                        const Ps2Vu1TerrainDrawResult vu1 = ps2_vu1_terrain_draw_slices(
                            *section.opaqueMesh, slices, sliceCount, batchVertices,
                            tile.tileX, tile.tileY, frame.native,
                            section.translateX, section.translateY, section.translateZ);
                        if (vu1.status == PS2_VU1_TERRAIN_SUBMITTED)
                        {
                            PS2_TERRAIN_CLUSTER_STAT(++s_clusterStats.vu1Ranges);
                            PS2_TERRAIN_CLUSTER_STAT(s_clusterStats.vu1Vertices += vu1.vertices);
#ifdef PS2_OPTIMIZATION_VALIDATION
                            Ps2OptimizationValidation::terrainVu1Submit(vu1.vertices);
#endif
                            result.nativeVertices += vu1.vertices;
                            sliceCount = 0;
                            batchVertices = 0;
                            return true;
                        }
#ifdef PS2_OPTIMIZATION_VALIDATION
                        Ps2OptimizationValidation::terrainVu1Retry(vu1.status == PS2_VU1_TERRAIN_FATAL);
#endif
                        s_currentVu1Retry = true;
                        if (vu1.status == PS2_VU1_TERRAIN_FATAL)
                        {
                            result.nativeVertices += vu1.vertices;
                            result.complete = false;
                            abortDraw = true;
                            return false;
                        }
                        if (s_opaqueSubmitPhase == PS2_OPAQUE_SUBMIT_VU1)
                        {
                            // Queued mode will replay the section through VU0.
                            abortDraw = true;
                            return false;
                        }
                        // Immediate mode cannot replay the whole section later.
                        // Preserve correctness by emitting just this compacted set
                        // through the packed VU0 path, then continue with later tiles.
                        ps2_render_release_path1();
                        for (int si = 0; si < sliceCount && !abortDraw; ++si)
                        {
                            const std::vector<Ps2TerrainTileRun>& clampRuns =
                                section.opaqueMesh->runs();
                            emitVu0Packed(section.opaqueMesh->positions(),
                                section.opaqueMesh->texCoords(), section.opaqueMesh->colors(),
                                slices[si].firstVertex, slices[si].vertexCount, true,
                                clampRuns.empty() ? nullptr : &clampRuns[0],
                                (int)clampRuns.size());
                        }
                        sliceCount = 0;
                        batchVertices = 0;
                        return !abortDraw;
                    };

                    for (std::size_t ri = rangeIndex; ri < ranges.size() && !abortDraw; ++ri)
                    {
                        const Ps2MeshRange& range = ranges[ri];
                        const int_t rangeBegin = range.firstVertex;
                        const int_t rangeEnd = rangeBegin + range.vertexCount();
                        if (rangeBegin >= tileEnd)
                            break;
                        if (rangeEnd <= tileBegin)
                            continue;

                        const int visibilityClass = clusterClass[range.cluster()];
                        const bool clipSafe = ps2_terrain_cluster_can_skip_clip(
                            visibilityClass, clusterGuardRisk[range.cluster()]);
                        if (visibilityClass == 0 || !clipSafe || !faceVisible(range.faceGroup()))
                            continue;

                        int_t cursor = rangeBegin > tileBegin ? rangeBegin : tileBegin;
                        const int_t endVertex = rangeEnd < tileEnd ? rangeEnd : tileEnd;
                        while (cursor < endVertex && !abortDraw)
                        {
                            if (sliceCount >= 8 || batchVertices >= PS2_VU1_TERRAIN_MAX_VERTICES)
                            {
                                if (!flushSlices())
                                    break;
                            }
                            int_t take = endVertex - cursor;
                            const int_t room = PS2_VU1_TERRAIN_MAX_VERTICES - batchVertices;
                            if (take > room) take = room;
                            take &= ~3;
                            if (take <= 0)
                            {
                                if (!flushSlices()) break;
                                continue;
                            }

                            if (sliceCount > 0 &&
                                slices[sliceCount - 1].firstVertex + slices[sliceCount - 1].vertexCount == cursor)
                            {
                                slices[sliceCount - 1].vertexCount += take;
                            }
                            else
                            {
                                if (sliceCount >= 8)
                                {
                                    if (!flushSlices()) break;
                                    continue;
                                }
                                slices[sliceCount].firstVertex = cursor;
                                slices[sliceCount].vertexCount = take;
                                ++sliceCount;
                            }
                            batchVertices += take;
                            cursor += take;
                        }
                    }
                    if (!abortDraw)
                        (void)flushSlices();
                }
                return;
            }
#endif
            // Fallback for raw meshes and experimental clipped-VU1 builds.
            int_t runStart = -1;
            int_t runCount = 0;
            bool runFullyInside = false;
            const std::vector<Ps2MeshRange>& ranges = section.faceGroups->ranges;
            for (std::size_t i = 0; i < ranges.size() && !abortDraw; ++i)
            {
                const Ps2MeshRange& range = ranges[i];
                const int visibilityClass = clusterClass[range.cluster()];
                const bool fullyInside = ps2_terrain_cluster_can_skip_clip(
                    visibilityClass, clusterGuardRisk[range.cluster()]);
                const bool keep = visibilityClass != 0 &&
                    (fullyInside || PS2_VU1_CLIPPED_PARTIALS) &&
                    faceVisible(range.faceGroup());
                if (keep)
                {
                    if (runStart >= 0 && runFullyInside == fullyInside &&
                        runStart + runCount == range.firstVertex)
                    {
                        runCount += range.vertexCount();
                    }
                    else
                    {
                        if (runStart >= 0)
                            emitRange(runStart, runCount, runFullyInside);
                        runStart = range.firstVertex;
                        runCount = range.vertexCount();
                        runFullyInside = fullyInside;
                    }
                }
                else if (runStart >= 0)
                {
                    emitRange(runStart, runCount, runFullyInside);
                    runStart = -1;
                    runCount = 0;
                }
            }
            if (!abortDraw && runStart >= 0)
                emitRange(runStart, runCount, runFullyInside);
        };

        auto emitGatheredRanges = [&](bool wantClipSafe)
        {
			if (!packedOpaque)
			{
				const std::vector<Ps2MeshRange>& ranges = section.faceGroups->ranges;
				for (std::size_t i = 0; i < ranges.size() && !abortDraw; ++i)
				{
					const Ps2MeshRange& range = ranges[i];
					const bool clipSafe = ps2_terrain_cluster_can_skip_clip(
						clusterClass[range.cluster()], clusterGuardRisk[range.cluster()]);
					if (clusterClass[range.cluster()] == 0 || clipSafe != wantClipSafe ||
						!faceVisible(range.faceGroup()))
						continue;
					PS2_TERRAIN_CLUSTER_STAT(++s_clusterStats.vu0GatherBatches);
					PS2_TERRAIN_CLUSTER_STAT(
						s_clusterStats.vu0GatherVertices += range.vertexCount());
					emitRange(range.firstVertex, range.vertexCount(), wantClipSafe);
				}
				return;
			}

            // Zero-copy gather: accumulate a bounded list of {firstVertex,
            // vertexCount} slices of the mesh's OWN arrays instead of
            // memcpy'ing them into a scratch buffer. Clamp runs are resolved
            // by Ps2Draw3D's clampRunCursor straight from the mesh's own tile
            // run table (clampRuns below), in the mesh's own vertex-index
            // space -- so unlike the old memcpy path, there is no rebasing to
            // a compacted buffer's local indices to track here at all.
            static const int kMaxGatherSlices = 32;
            Ps2NativeSlice gatherSlices[kMaxGatherSlices];
            int gatherSliceCount = 0;
            int_t gatherVertices = 0;
            const std::vector<Ps2MeshRange>& ranges = section.faceGroups->ranges;
            const std::vector<Ps2TerrainTileRun>& clampRuns = section.opaqueMesh->runs();

            auto flushGather = [&]()
            {
                if (gatherVertices <= 0 || abortDraw)
                    return;
                PS2_TERRAIN_CLUSTER_STAT(++s_clusterStats.vu0GatherBatches);
                PS2_TERRAIN_CLUSTER_STAT(s_clusterStats.vu0GatherVertices += gatherVertices);
                emitVu0PackedSliced(section.opaqueMesh->positions(),
                    section.opaqueMesh->texCoords(), section.opaqueMesh->colors(),
                    gatherSlices, gatherSliceCount, gatherVertices, wantClipSafe,
                    clampRuns.empty() ? nullptr : &clampRuns[0], (int)clampRuns.size());
                gatherSliceCount = 0;
                gatherVertices = 0;
            };

            const int_t maxBatchVertices = ps2_terrain_max_batch_vertices();

            // Quad-aligned by construction: every range is a whole number of
            // quads, the batch bound is a multiple of 4, and every `take`
            // below is rounded down to one -- so a split slice can only end
            // on a quad boundary, never mid-quad.
            auto appendSlice = [&](int_t firstVertex, int_t vertexCount)
            {
                while (vertexCount > 0 && !abortDraw)
                {
                    const bool canMerge = gatherSliceCount > 0 &&
                        gatherSlices[gatherSliceCount - 1].firstVertex +
                            gatherSlices[gatherSliceCount - 1].vertexCount == firstVertex;
                    if (!canMerge && gatherSliceCount >= kMaxGatherSlices)
                    {
                        flushGather();
                        continue;
                    }

                    int_t take = maxBatchVertices - gatherVertices;
                    if (take > vertexCount) take = vertexCount;
                    take -= take & 3;
                    if (take <= 0)
                    {
                        flushGather();
                        continue;
                    }

                    if (canMerge)
                    {
                        gatherSlices[gatherSliceCount - 1].vertexCount += (int)take;
                    }
                    else
                    {
                        gatherSlices[gatherSliceCount].firstVertex = (int)firstVertex;
                        gatherSlices[gatherSliceCount].vertexCount = (int)take;
                        ++gatherSliceCount;
                    }
                    gatherVertices += take;
                    firstVertex += take;
                    vertexCount -= take;
                }
            };

            for (std::size_t i = 0; i < ranges.size() && !abortDraw; ++i)
            {
                const Ps2MeshRange& range = ranges[i];
                const bool clipSafe = ps2_terrain_cluster_can_skip_clip(
                    clusterClass[range.cluster()],
                    clusterGuardRisk[range.cluster()]);
                const bool submittedByVu1 = directVu1Usable &&
                    (clipSafe || PS2_VU1_CLIPPED_PARTIALS);
                if (clusterClass[range.cluster()] == 0 ||
                    clipSafe != wantClipSafe ||
                    submittedByVu1 ||
                    !faceVisible(range.faceGroup()))
                    continue;

                appendSlice(range.firstVertex, range.vertexCount());
            }

            flushGather();
        };

		if (directVu1Usable && s_opaqueSubmitPhase != PS2_OPAQUE_SUBMIT_VU0)
            emitVu1Ranges();
		if (!abortDraw && directVu1Usable &&
			s_opaqueSubmitPhase == PS2_OPAQUE_SUBMIT_VU1 &&
			ps2_vu1_terrain_clipped_probe_available())
		{
			const std::vector<Ps2MeshRange>& ranges = section.faceGroups->ranges;
			for (std::size_t i = 0; i < ranges.size() &&
				ps2_vu1_terrain_clipped_probe_available(); ++i)
			{
				const Ps2MeshRange& range = ranges[i];
				const int visibilityClass = clusterClass[range.cluster()];
				const bool fullyInside = ps2_terrain_cluster_can_skip_clip(
					visibilityClass, clusterGuardRisk[range.cluster()]);
				if (visibilityClass != 0 && !fullyInside &&
					faceVisible(range.faceGroup()))
				{
					emitClippedProbe(range.firstVertex, range.vertexCount());
				}
			}
		}
		if (!abortDraw && s_opaqueSubmitPhase != PS2_OPAQUE_SUBMIT_VU1)
            emitGatheredRanges(true);
		if (!abortDraw && s_opaqueSubmitPhase != PS2_OPAQUE_SUBMIT_VU1)
            emitGatheredRanges(false);
    }
    else
    {
		if (!section.fullyInside)
			emitClippedProbe(0, totalVertices);

		// With no classified ranges the whole mesh is one VU1 range. Its VU0
		// phase is needed only when that VU1 submission explicitly asked to retry.
		const bool submittedByVu1 = directVu1Usable &&
			(section.fullyInside || PS2_VU1_CLIPPED_PARTIALS);
		if (!(s_opaqueSubmitPhase == PS2_OPAQUE_SUBMIT_VU0 && submittedByVu1))
			emitRange(0, totalVertices, section.fullyInside);
    }

    return result;
}


#endif
