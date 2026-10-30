#include "ps3/spu/Ps3SpuChunkMesh.h"

#if PLATFORM_SPU_CHUNK_MESH

#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <malloc.h>

#include "client/Minecraft.h"
#include "net/minecraft/src/Block.h"
#include "net/minecraft/src/BlockGrass.h"
#include "net/minecraft/src/Config.h"
#include "net/minecraft/src/ConnectedTextures.h"
#include "net/minecraft/src/CustomColorizer.h"
#include "net/minecraft/src/EntityRenderer.h"
#include "net/minecraft/src/Material.h"
#include "net/minecraft/src/NaturalTextures.h"
#include "net/minecraft/src/RenderBlocks.h"
#include "net/minecraft/src/Tessellator.h"
#include "pc/render/PcLegacyBlockRenderInfo.h"
#include "pc/render/PcLegacySectionCache.h"
#include "platform/Log.h"
#include "platform/PlatformCompat.h"
#include "platform/PlatformTuning.h"
#include "ps3/spu/Ps3Spu.h"
#include "ps3/spu/tasks/ChunkMesh/SpuChunkMeshJob.h"

#ifndef PS3_SPU_MESH_VERIFY_SECTIONS
#define PS3_SPU_MESH_VERIFY_SECTIONS 64
#endif
#ifndef PS3_SPU_MESH_OUTPUT_VERTICES
#define PS3_SPU_MESH_OUTPUT_VERTICES 12288
#endif

static_assert(sizeof(int_t) == 4, "the SPU writes 32-bit vertex ints");
static_assert(Block::BLOCK_REGISTRY_SIZE <= SPU_MESH_IDS, "SPU mesh tables index block ids in 8 bits");
static_assert(PcLegacySectionCache::kSize == SPU_MESH_SIZE, "snapshot size");

namespace
{
    struct Ticket
    {
        SpuChunkMeshInput input __attribute__((aligned(128)));
        Ps3SpuJob job;
        std::uint32_t *output = nullptr;   // PS3_SPU_MESH_OUTPUT_VERTICES * 8 ints, 128-aligned
        bool active = false;
        std::uint64_t submitUs = 0;
    };

    Ticket s_tickets[kPs3SpuMeshTickets];

    SpuMeshTextureTable s_textures __attribute__((aligned(128)));
    bool s_texturesReady = false;
    // Block ids whose texture table entry is usable (every tile < 256).
    bool s_textureOk[SPU_MESH_IDS];

    // Kind per id, recomputed when the settings that decide it change.
    unsigned char s_kind[SPU_MESH_IDS];
    std::uint32_t s_kindSignature = 0xffffffffu;
    int s_kindAge = 0;

    std::atomic<bool> s_disabled{false};
    int s_verifyRemaining = PS3_SPU_MESH_VERIFY_SECTIONS;
    Tessellator *s_verifyTessellator = nullptr;

    // Counters (worker writes, main thread reads and resets).
    std::atomic<long> s_statSections{0};
    std::atomic<long> s_statSpuTicks{0};
    std::atomic<long> s_statWaitMicros{0};
    std::atomic<long> s_statRoundTripMicros{0};
    std::atomic<long> s_statSpuBlocks{0};
    std::atomic<long> s_statSpuFaces{0};
    std::atomic<long> s_statPpuBlocks{0};
    std::atomic<long> s_statFallbacks{0};
    std::atomic<long> s_statOverflows{0};
    std::atomic<long> s_statVerified{0};
    std::atomic<long> s_statVerifyMismatches{0};
    std::atomic<bool> s_loggedStart{false};
    std::uint64_t s_statLastLogUs = 0;

    bool tileIsPlain(int_t blockId, int_t tile)
    {
        if (tile < 0 || tile > 255)
            return false;
        if (ConnectedTextures::mayChangeTexture(blockId, tile))
            return false;
        if (Config::isNaturalTextures() && NaturalTextures::getNaturalProperties(0, tile) != nullptr)
            return false;
        return true;
    }

    void buildTextures()
    {
        if (s_texturesReady)
            return;
        std::memset(&s_textures, 0, sizeof(s_textures));
        for (int_t id = 0; id < Block::BLOCK_REGISTRY_SIZE; ++id)
        {
            const PcLegacyBlockRenderInfo &info = pcLegacyGetBlockRenderInfo(id);
            bool ok = info.staticTextureByMetadata;
            for (int meta = 0; ok && meta < 16; ++meta)
            {
                for (int side = 0; side < 6; ++side)
                {
                    const int tile = info.textureByMetadata[static_cast<std::size_t>(meta)][static_cast<std::size_t>(side)];
                    if (tile < 0 || tile > 255)
                    {
                        ok = false;
                        break;
                    }
                    s_textures.tile[(id * 16 + meta) * 6 + side] = static_cast<unsigned char>(tile);
                }
            }
            s_textureOk[id] = ok;
        }
        s_texturesReady = true;
    }

    std::uint32_t kindSignature()
    {
        return (Config::isConnectedTextures() ? 1u : 0u) | (Config::isNaturalTextures() ? 2u : 0u) |
            (Config::isBetterGrass() ? 4u : 0u) | (RenderBlocks::fancyGrass ? 8u : 0u);
    }

    unsigned char computeKind(int_t id)
    {
        Block *block = Block::blocksList[id];
        if (block == nullptr || Block::lightValue[id] != 0)
            return SPU_MESH_KIND_NONE;
        const PcLegacyBlockRenderInfo &info = pcLegacyGetBlockRenderInfo(id);
        if (!info.simpleOpaqueCube)
            return SPU_MESH_KIND_NONE;
        for (int meta = 0; meta < 16; ++meta)
            if (CustomColorizer::hasBlockPalette(id, meta))
                return SPU_MESH_KIND_NONE;

        if (block == static_cast<Block *>(Block::grass))
        {
            if (Config::isBetterGrass())
                return SPU_MESH_KIND_NONE;
            static const int kGrassTiles[] = {0, 2, 3, 68, 38};
            for (int tile : kGrassTiles)
                if (!tileIsPlain(id, tile))
                    return SPU_MESH_KIND_NONE;
            return SPU_MESH_KIND_GRASS;
        }

        if (!info.staticTextureByMetadata || !info.defaultWhiteColorMultiplier || !s_textureOk[id] ||
            block->blockIndexInTexture == 3)
            return SPU_MESH_KIND_NONE;
        for (int meta = 0; meta < 16; ++meta)
        {
            for (int side = 0; side < 6; ++side)
            {
                const int tile = s_textures.tile[(id * 16 + meta) * 6 + side];
                // Grass-side tiles take the better-grass / fancy-overlay paths.
                if (side >= 2 && (tile == 3 || tile == 68 || tile == 77))
                    return SPU_MESH_KIND_NONE;
                if (!tileIsPlain(id, tile))
                    return SPU_MESH_KIND_NONE;
            }
        }
        return SPU_MESH_KIND_CUBE;
    }

    void refreshKinds()
    {
        buildTextures();
        const std::uint32_t signature = kindSignature();
        // Resource reloads can change CTM properties and palettes without a
        // settings change: recompute now and then anyway.
        if (signature == s_kindSignature && ++s_kindAge < 512)
            return;
        s_kindSignature = signature;
        s_kindAge = 0;
        for (int_t id = 0; id < SPU_MESH_IDS; ++id)
            s_kind[id] = id < Block::BLOCK_REGISTRY_SIZE ? computeKind(id) : SPU_MESH_KIND_NONE;
    }

    unsigned char blockFlags(int_t id)
    {
        unsigned char flags = 0;
        if (Block::canBlockGrass[id])
            flags |= SPU_MESH_FLAG_BLOCKS_GRASS;
        if (Block::useNeighborBrightness[id])
            flags |= SPU_MESH_FLAG_NEIGHBOR_LIGHT;
        Block *block = Block::blocksList[id];
        if (block == nullptr)
            return flags;
        // PcLegacySectionCache::isBlockOpaqueCube.
        if (Block::staticOpaqueCubeLookupSafe[id] ? Block::opaqueCubeLookup[id] : block->isOpaqueCube())
            flags |= SPU_MESH_FLAG_OPAQUE;
        // RenderBlocks AO: calculateAoBrightness.
        if (block != Block::glass && Block::isNormalCube(id))
            flags |= SPU_MESH_FLAG_AO_DARK;
        if (block->blockMaterial == Material::snow || block->blockMaterial == Material::builtSnow)
            flags |= SPU_MESH_FLAG_SNOW;
        return flags;
    }

    bool globallyAllowed()
    {
        return !s_disabled.load(std::memory_order_relaxed) && Minecraft::isAmbientOcclusionEnabled() &&
            !EntityRenderer::anaglyphEnabled && !Tessellator::convertQuadsToTriangles && ps3SpuAvailable();
    }

    // The PPU path for the SPU's blocks: the worker's loop restricted to them.
    // Used to verify the SPU and as the fallback.
    int renderSpuBlocksOnPpu(Ticket &ticket, PcLegacySectionCache &cache, Tessellator &tessellator,
        int_t x0, int_t y0, int_t z0)
    {
        RenderBlocks renderblocks(&cache);
        renderblocks.setMeshTessellator(&tessellator);
        const int_t workCount = cache.getRenderWorkCount(0);
        int blocks = 0;
        for (int_t work = 0; work < workCount; ++work)
        {
            const int_t cursor = static_cast<int_t>(cache.getRenderWorkEntry(0, work));
            const int_t lx = cursor & 15;
            const int_t lz = (cursor >> 4) & 15;
            const int_t ly = (cursor >> 8) & 15;
            const int_t id = cache.getBlockIdLocal(lx, ly, lz);
            if (id <= 0 || id >= Block::BLOCK_REGISTRY_SIZE || ticket.input.kind[id] == SPU_MESH_KIND_NONE)
                continue;
            Block *block = Block::blocksList[id];
            const std::uint8_t faceMask = cache.getExposedFaceMaskLocal(lx, ly, lz);
            if (block == nullptr || faceMask == 0)
                continue;
            renderblocks.renderSimpleOpaqueCubeLegacy(block, x0 + lx, y0 + ly, z0 + lz, faceMask);
            ++blocks;
        }
        return blocks;
    }

    const char *fieldName(int field)
    {
        static const char *const kNames[8] = {"x", "y", "z", "u", "v", "colour", "normal", "brightness"};
        return field >= 0 && field < 8 ? kNames[field] : "?";
    }

    // Compares the SPU's vertices with the PPU's (all fields but the unused
    // normal). True when they are equal.
    bool verify(Ticket &ticket, PcLegacySectionCache &cache, int_t x0, int_t y0, int_t z0)
    {
        if (s_verifyTessellator == nullptr)
            s_verifyTessellator = new Tessellator(PLATFORM_TESSELLATOR_BUFFER_INTS);
        Tessellator &tess = *s_verifyTessellator;
        tess.startDrawingQuads();
        tess.setTranslationD(-static_cast<double>(x0), -static_cast<double>(y0), -static_cast<double>(z0));
        renderSpuBlocksOnPpu(ticket, cache, tess, x0, y0, z0);

        const int ppuCount = static_cast<int>(tess.pendingVertexCount());
        const int spuCount = static_cast<int>(ticket.input.resultVertices);
        const std::uint32_t *ppu = reinterpret_cast<const std::uint32_t *>(tess.pendingRawVertices());
        const std::uint32_t *spu = ticket.output;
        bool equal = ppuCount == spuCount;
        int badVertex = -1;
        int badField = -1;
        if (equal)
        {
            for (int v = 0; v < ppuCount && equal; ++v)
            {
                for (int field = 0; field < 8; ++field)
                {
                    if (field == 6)
                        continue;
                    if (ppu[v * 8 + field] != spu[v * 8 + field])
                    {
                        equal = false;
                        badVertex = v;
                        badField = field;
                        break;
                    }
                }
            }
        }

        s_statVerified.fetch_add(1, std::memory_order_relaxed);
        if (!equal)
        {
            s_statVerifyMismatches.fetch_add(1, std::memory_order_relaxed);
            if (badVertex < 0)
            {
                MC_LOG_WARN("ps3.spumesh", "verify: mismatch at section %d,%d,%d: PPU %d vertices, SPU %d\n",
                    (int)x0, (int)y0, (int)z0, ppuCount, spuCount);
            }
            else
            {
                const std::uint32_t *p = &ppu[badVertex * 8];
                const std::uint32_t *s = &spu[badVertex * 8];
                float px, py, pz;
                std::memcpy(&px, &p[0], 4);
                std::memcpy(&py, &p[1], 4);
                std::memcpy(&pz, &p[2], 4);
                MC_LOG_WARN("ps3.spumesh",
                    "verify: mismatch at section %d,%d,%d vertex %d (face %d corner %d, local %.0f,%.0f,%.0f) "
                    "field %s: PPU 0x%08x SPU 0x%08x; PPU vtx %08x %08x %08x %08x %08x %08x %08x, "
                    "SPU vtx %08x %08x %08x %08x %08x %08x %08x\n",
                    (int)x0, (int)y0, (int)z0, badVertex, badVertex / 4, badVertex % 4, px, py, pz,
                    fieldName(badField), (unsigned)p[badField], (unsigned)s[badField],
                    (unsigned)p[0], (unsigned)p[1], (unsigned)p[2], (unsigned)p[3], (unsigned)p[4], (unsigned)p[5],
                    (unsigned)p[7], (unsigned)s[0], (unsigned)s[1], (unsigned)s[2], (unsigned)s[3], (unsigned)s[4],
                    (unsigned)s[5], (unsigned)s[7]);
            }
        }
        tess.cancelDrawing();
        tess.setTranslationD(0.0, 0.0, 0.0);
        return equal;
    }
}

bool ps3SpuMeshBegin(int slot, PcLegacySectionCache &cache)
{
    if (slot < 0 || slot >= kPs3SpuMeshTickets || !cache.isDetached() || !globallyAllowed())
        return false;
    Ticket &ticket = s_tickets[slot];
    ps3SpuMeshCancel(slot);

    const int_t workCount = cache.getRenderWorkCount(0);
    if (workCount <= 0 || workCount > SPU_MESH_MAX_WORK)
        return false;

    if (ticket.output == nullptr)
    {
        ticket.output = static_cast<std::uint32_t *>(
            memalign(128, static_cast<std::size_t>(PS3_SPU_MESH_OUTPUT_VERTICES) * SPU_MESH_VERTEX_INTS * 4));
        if (ticket.output == nullptr)
            return false;
    }

    refreshKinds();
    SpuChunkMeshInput &in = ticket.input;
    std::memcpy(in.kind, s_kind, sizeof(in.kind));
    for (int_t id = 0; id < SPU_MESH_IDS; ++id)
        in.flags[id] = id < Block::BLOCK_REGISTRY_SIZE ? blockFlags(id) : 0;

    // Anything for the SPU here? Also note the columns that need grass colour.
    bool any = false;
    std::uint8_t grassColumn[256] = {};
    bool anyGrass = false;
    for (int_t work = 0; work < workCount; ++work)
    {
        const int_t cursor = static_cast<int_t>(cache.getRenderWorkEntry(0, work));
        const int_t lx = cursor & 15;
        const int_t lz = (cursor >> 4) & 15;
        const int_t ly = (cursor >> 8) & 15;
        const int_t id = cache.getBlockIdLocal(lx, ly, lz);
        if (id <= 0 || id >= Block::BLOCK_REGISTRY_SIZE)
            continue;
        const unsigned char kind = in.kind[id];
        if (kind == SPU_MESH_KIND_NONE)
            continue;
        any = true;
        if (kind == SPU_MESH_KIND_GRASS)
        {
            grassColumn[lz * 16 + lx] = static_cast<std::uint8_t>(ly + 1);
            anyGrass = true;
        }
    }
    if (!any)
        return false;

    const int_t x0 = cache.originX() + 1;
    const int_t y0 = cache.originY() + 1;
    const int_t z0 = cache.originZ() + 1;
    std::memset(in.grassColor, 0, sizeof(in.grassColor));
    if (anyGrass)
    {
        for (int column = 0; column < 256; ++column)
        {
            if (grassColumn[column] == 0)
                continue;
            const int_t lx = column & 15;
            const int_t lz = column >> 4;
            const int_t ly = grassColumn[column] - 1;
            in.grassColor[column] = CustomColorizer::getColorMultiplier(Block::grass, &cache,
                x0 + lx, y0 + ly, z0 + lz);
        }
    }

    // RenderBlocks reads its AO constants from Config at construction.
    RenderBlocks reference(&cache);
    in.eaBlockIds = reinterpret_cast<std::uintptr_t>(cache.rawBlockIds());
    in.eaMetadata = reinterpret_cast<std::uintptr_t>(cache.rawMetadata());
    in.eaSavedLight = reinterpret_cast<std::uintptr_t>(cache.rawSavedLight());
    in.eaWorklist = reinterpret_cast<std::uintptr_t>(cache.rawRenderWorkset(0));
    in.eaTextures = reinterpret_cast<std::uintptr_t>(&s_textures);
    in.eaOutput = reinterpret_cast<std::uintptr_t>(ticket.output);
    in.workCount = static_cast<unsigned int>(workCount);
    in.outputCapacity = PS3_SPU_MESH_OUTPUT_VERTICES;
    in.texturesVersion = 1;
    in.minY = static_cast<int>(cache.originY());
    in.aoLightValueOpaque = reference.aoLightValueOpaque;
    in.lightingQuality = static_cast<int>(reference.lightingQuality);
    in.fancyGrass = RenderBlocks::fancyGrass ? 1 : 0;
    in.pad0 = 0;
    in.resultVertices = 0;
    in.resultStatus = 0xffffffffu;
    in.resultBlocks = 0;
    in.resultFaces = 0;

    const std::uintptr_t eas[] = {in.eaBlockIds, in.eaMetadata, in.eaSavedLight, in.eaWorklist, in.eaOutput};
    for (std::uintptr_t ea : eas)
        if ((ea & 15) != 0 || (ea >> 32) != 0)
            return false;

    ticket.submitUs = PlatformCompat::getMonotonicMicros();
    if (!ps3SpuSubmit(ticket.job, PS3_SPU_JOB_CHUNK_MESH, &ticket.input))
        return false;
    ticket.active = true;
    if (!s_loggedStart.exchange(true))
        MC_LOG_INFO("ps3.spumesh", "first section on the SPUs (%d work entries); verifying the first %d sections\n",
            (int)workCount, PS3_SPU_MESH_VERIFY_SECTIONS);
    return true;
}

void ps3SpuMeshCancel(int slot)
{
    if (slot < 0 || slot >= kPs3SpuMeshTickets)
        return;
    Ticket &ticket = s_tickets[slot];
    if (!ticket.active)
        return;
    // The SPU may still be reading the snapshot: let it finish, drop the result.
    ps3SpuWait(ticket.job);
    ticket.active = false;
}

bool ps3SpuMeshHandles(int slot, int_t blockId)
{
    if (slot < 0 || slot >= kPs3SpuMeshTickets || blockId <= 0 || blockId >= SPU_MESH_IDS)
        return false;
    const Ticket &ticket = s_tickets[slot];
    return ticket.active && ticket.input.kind[blockId] != SPU_MESH_KIND_NONE;
}

void ps3SpuMeshEnd(int slot, PcLegacySectionCache &cache, Tessellator &tessellator, int_t x0, int_t y0, int_t z0)
{
    if (slot < 0 || slot >= kPs3SpuMeshTickets)
        return;
    Ticket &ticket = s_tickets[slot];
    if (!ticket.active)
        return;
    ticket.active = false;

    const std::uint64_t waitStart = PlatformCompat::getMonotonicMicros();
    const bool done = ps3SpuWait(ticket.job);
    const std::uint64_t waitEnd = PlatformCompat::getMonotonicMicros();
    s_statWaitMicros.fetch_add(static_cast<long>(waitEnd - waitStart), std::memory_order_relaxed);
    s_statRoundTripMicros.fetch_add(static_cast<long>(waitEnd - ticket.submitUs), std::memory_order_relaxed);
    s_statSpuTicks.fetch_add(static_cast<long>(ticket.job.header.spuTicks), std::memory_order_relaxed);
    s_statSections.fetch_add(1, std::memory_order_relaxed);

    // The SPU's result block landed before its header status said done.
    const SpuChunkMeshInput &in = ticket.input;
    bool useSpu = done && in.resultStatus == SPU_MESH_STATUS_OK;
    if (done && in.resultStatus == SPU_MESH_STATUS_OVERFLOW)
        s_statOverflows.fetch_add(1, std::memory_order_relaxed);

    if (useSpu && s_verifyRemaining > 0)
    {
        --s_verifyRemaining;
        if (!verify(ticket, cache, x0, y0, z0))
        {
            useSpu = false;
            s_disabled.store(true, std::memory_order_relaxed);
            MC_LOG_WARN("ps3.spumesh", "SPU vertices differ from the PPU's; sections stay on the PPU worker\n");
        }
        else if (s_verifyRemaining == 0)
        {
            MC_LOG_INFO("ps3.spumesh", "verify: %d sections bit-for-bit equal; SPU meshing only from now on\n",
                PS3_SPU_MESH_VERIFY_SECTIONS);
        }
    }

    if (useSpu && in.resultVertices > 0 &&
        !tessellator.appendRawQuadVertices(ticket.output, static_cast<int>(in.resultVertices)))
        useSpu = false;

    if (useSpu)
    {
        s_statSpuBlocks.fetch_add(static_cast<long>(in.resultBlocks), std::memory_order_relaxed);
        s_statSpuFaces.fetch_add(static_cast<long>(in.resultFaces), std::memory_order_relaxed);
        return;
    }

    s_statFallbacks.fetch_add(1, std::memory_order_relaxed);
    const int blocks = renderSpuBlocksOnPpu(ticket, cache, tessellator, x0, y0, z0);
    s_statPpuBlocks.fetch_add(blocks, std::memory_order_relaxed);
}

void ps3SpuMeshNotePpuBlocks(int blocks)
{
    s_statPpuBlocks.fetch_add(blocks, std::memory_order_relaxed);
}

void ps3SpuMeshLogCounters()
{
    const std::uint64_t nowUs = PlatformCompat::getMonotonicMicros();
    if (s_statLastLogUs == 0)
    {
        s_statLastLogUs = nowUs;
        return;
    }
    if (nowUs - s_statLastLogUs < 5u * 1000u * 1000u)
        return;
    s_statLastLogUs = nowUs;

    const long sections = s_statSections.exchange(0, std::memory_order_relaxed);
    const long ppuBlocks = s_statPpuBlocks.exchange(0, std::memory_order_relaxed);
    if (sections == 0 && ppuBlocks == 0)
        return;
    const long spuTicks = s_statSpuTicks.exchange(0, std::memory_order_relaxed);
    const long waitUs = s_statWaitMicros.exchange(0, std::memory_order_relaxed);
    const long roundTripUs = s_statRoundTripMicros.exchange(0, std::memory_order_relaxed);
    const long spuBlocks = s_statSpuBlocks.exchange(0, std::memory_order_relaxed);
    const long spuFaces = s_statSpuFaces.exchange(0, std::memory_order_relaxed);
    const long fallbacks = s_statFallbacks.exchange(0, std::memory_order_relaxed);
    const long overflows = s_statOverflows.exchange(0, std::memory_order_relaxed);
    const long verified = s_statVerified.exchange(0, std::memory_order_relaxed);
    const long mismatches = s_statVerifyMismatches.exchange(0, std::memory_order_relaxed);
    // SPU decrementer = timebase, 79.8 MHz.
    const long spuUs = static_cast<long>(static_cast<long long>(spuTicks) * 10 / 798);
    const long totalBlocks = spuBlocks + ppuBlocks;
    const char *state = s_disabled.load(std::memory_order_relaxed) ? "OFF (verify mismatch)"
        : (s_verifyRemaining > 0 ? "verifying" : "verified");
    MC_LOG_INFO("ps3.spumesh",
        "sections=%ld spuUs=%ld (%ld us/section) waitUs=%ld (%ld us/section) roundTrip=%ld us/section "
        "blocks: spu=%ld faces=%ld ppu=%ld (leftover %ld%%) fallbacks=%ld overflows=%ld verify=%ld/%ld bad "
        "state=%s threads=%d\n",
        sections, spuUs, sections > 0 ? spuUs / sections : 0L, waitUs, sections > 0 ? waitUs / sections : 0L,
        sections > 0 ? roundTripUs / sections : 0L, spuBlocks, spuFaces, ppuBlocks,
        totalBlocks > 0 ? ppuBlocks * 100 / totalBlocks : 0L, fallbacks, overflows, mismatches, verified, state,
        ps3SpuThreadCount());
}

#endif
