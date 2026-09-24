#include "platform/PlatformConfig.h"

#if PLATFORM_INCREMENTAL_TERRAIN_BUILD

#include "net/minecraft/src/WorldRenderer.h"

#include <algorithm>
#include <cstdint>
#include <vector>

#include "java/Arithmetic.h"
#include "net/minecraft/src/Block.h"
#include "net/minecraft/src/Chunk.h"
#include "net/minecraft/src/ConnectedTextures.h"
#include "net/minecraft/src/ExtendedBlockStorage.h"
#include "net/minecraft/src/RenderBlocks.h"
#include "net/minecraft/src/Tessellator.h"
#include "net/minecraft/src/TileEntity.h"
#include "net/minecraft/src/TileEntityRenderer.h"
#include "net/minecraft/src/OpenGlHelper.h"
#include "net/minecraft/src/World.h"
#include "pc/render/PcLegacyTerrainStaging.h"
#include "pc/render/PcLegacyBlockRenderInfo.h"
#include "pc/render/PcLegacyStaticTileEntityMesh.h"
#include "pc/render/PcLegacyStaticTileEntityPolicy.h"
#include "platform/PlatformCompat.h"
#include "platform/PlatformTuning.h"
#include "pc/tuning/PcLegacyTuning.h"
#include "platform/RenderAPI.h"

namespace
{
    unsigned int pcLegacySourceAvailability(World *world, int_t x0, int_t z0, int_t x1, int_t z1)
    {
        unsigned int availability = 0u;
        unsigned int bit = 1u;
        const int_t ccx0 = JavaArithmetic::intShr(x0 - 1, 4);
        const int_t ccx1 = JavaArithmetic::intShr(x1 + 1, 4);
        const int_t ccz0 = JavaArithmetic::intShr(z0 - 1, 4);
        const int_t ccz1 = JavaArithmetic::intShr(z1 + 1, 4);
        for (int_t ccx = ccx0; ccx <= ccx1; ++ccx)
        {
            for (int_t ccz = ccz0; ccz <= ccz1; ++ccz, bit <<= 1)
            {
                if (world->chunkExists(ccx, ccz))
                    availability |= bit;
            }
        }
        return availability;
    }

    bool pcLegacyStepTimeExpired(std::uint64_t stepStartUs)
    {
        if (PLATFORM_CHUNK_BUILD_STEP_US <= 0)
            return false;
        const std::uint64_t nowUs = PlatformCompat::getMonotonicMicros();
        return nowUs > stepStartUs &&
            nowUs - stepStartUs >= static_cast<std::uint64_t>(PLATFORM_CHUNK_BUILD_STEP_US);
    }

    bool pcLegacyCentralSectionIsEmpty(World *world, int_t x, int_t y, int_t z)
    {
        if (world == nullptr || y < 0)
            return false;

        const int_t chunkX = JavaArithmetic::intShr(x, 4);
        const int_t chunkZ = JavaArithmetic::intShr(z, 4);
        if (!world->chunkExists(chunkX, chunkZ))
            return false;

        Chunk *chunk = world->getChunkFromChunkCoords(chunkX, chunkZ);
        if (chunk == nullptr || chunk->isEmptyChunk())
            return false;

        const ExtendedBlockStorage *storage = chunk->getBlockStorage(JavaArithmetic::intShr(y, 4));
        return storage == nullptr || storage->getIsEmpty();
    }
}

void WorldRenderer::updateRenderer()
{
    pcLegacyStepDidWork = false;
    if (!needsUpdate)
    {
        if (pcLegacyBuildActive)
            pcLegacyResetBuildState();
        return;
    }

    pcLegacyBuildRendererStep(PLATFORM_CHUNK_BUILD_BLOCKS_PER_STEP);
}

void WorldRenderer::pcLegacyResetBuildState()
{
    pcLegacyTerrainStagingRelease(this);
    pcLegacyBuildActive = false;
    pcLegacyBuildSourceAvailability = 0u;
    pcLegacyBuildSourceAvailabilityValid = false;
    pcLegacyBuildPass = 0;
    pcLegacyBuildCursor = 0;
    pcLegacyBuildHasPass1 = false;
    pcLegacyBuildChunkLit = false;
    pcLegacyStepDidWork = false;
    pcLegacyBuildDirtyDuringBuild = false;
}

bool WorldRenderer::pcLegacyBeginBuildState()
{
    pcLegacyResetBuildState();
    if (worldObj == nullptr)
        return false;

    const int_t x1 = posX + sizeWidth;
    const int_t y1 = posY + sizeHeight;
    const int_t z1 = posZ + sizeDepth;
    if (!pcLegacyTerrainStagingAcquire(this, worldObj, posX, posY, posZ, x1, y1, z1))
        return false;

    pcLegacyBuildActive = true;
    isVisibleFromPosition = false;
    return true;
}

bool WorldRenderer::isTerrainBuildInProgress() const
{
    return pcLegacyBuildActive;
}

bool WorldRenderer::lastTerrainBuildStepDidWork() const
{
    return pcLegacyStepDidWork;
}

std::uint8_t WorldRenderer::pcLegacyVisibleFacesFrom(int_t face) const
{
    if (!isInitialized || face < 0 || face >= 6)
        return 0x3f;
    return pcLegacyPublishedVisibility[face];
}

void WorldRenderer::pcLegacyPublishBuild(PcLegacyTerrainStaging &staging)
{
    std::vector<TileEntity *> rebuiltStaticTileEntityRenderers;

#if PC_LEGACY_STATIC_TILE_ENTITY_MESH && PLATFORM_PC_LEGACY
    // ModelRenderer itself owns per-part display lists on desktop. Compile those
    // before opening the chunk display list; OpenGL does not allow glNewList to
    // be nested inside another glNewList.
    if (worldObj != nullptr)
    {
        for (const PcLegacyStaticTileEntityCandidate &candidate : staging.staticTileEntityCandidates)
        {
            TileEntity *te = candidate.tileEntity;
            if (te == nullptr || te->isInvalid())
                continue;
            TileEntity *current = worldObj->getBlockTileEntity(candidate.x, candidate.y, candidate.z);
            if (!pcLegacyStaticTileEntityCandidateIsCurrent(candidate, current))
                continue;
            (void)TileEntityRenderer::instance.prepareStaticPart(te);
        }
    }
#endif

    for (int_t pass = 0; pass < 2; ++pass)
    {
        renderBeginDisplayList(glRenderList + pass);

        // Terrain keeps the tiny vanilla/OptiFine seam scale. Static TileEntity
        // geometry is emitted later under a separate matrix so it never inherits
        // this scale (otherwise sign text/chest lids can misalign).
        renderPushMatrix();
        renderTranslate(static_cast<float>(posXClip), static_cast<float>(posYClip), static_cast<float>(posZClip));
        const float terrainScale = pcLegacyTerrainListScale();
        renderTranslate(-static_cast<float>(sizeDepth) / 2.0f,
            -static_cast<float>(sizeHeight) / 2.0f,
            -static_cast<float>(sizeDepth) / 2.0f);
        renderScale(terrainScale, terrainScale, terrainScale);
        renderTranslate(static_cast<float>(sizeDepth) / 2.0f,
            static_cast<float>(sizeHeight) / 2.0f,
            static_cast<float>(sizeDepth) / 2.0f);

        const bool compactDrawn = renderDrawCaptured(staging.compactPassMeshes[pass]);
        const bool baseDrawn = renderDrawCaptured(staging.passMeshes[pass]);
        bool extraDrawn = false;
        for (const TessellatorTextureMesh &group : staging.extraTextureMeshes[pass])
        {
            if (group.mesh.empty())
                continue;
            renderBindTexture(group.textureId);
            extraDrawn |= renderDrawCaptured(group.mesh);
        }
        if (extraDrawn)
            renderBindTexture(ConnectedTextures::getTerrainTextureId());
        renderPopMatrix();

        bool staticTileDrawn = false;
#if PC_LEGACY_STATIC_TILE_ENTITY_MESH && PLATFORM_PC_LEGACY
        if (pass == 0 && worldObj != nullptr && !staging.staticTileEntityCandidates.empty())
        {
            renderPushMatrix();
            renderTranslate(static_cast<float>(posXClip), static_cast<float>(posYClip), static_cast<float>(posZClip));
            const float staticScale = pcLegacyStaticTileEntityListScale();
            renderScale(staticScale, staticScale, staticScale);

            for (const PcLegacyStaticTileEntityCandidate &candidate : staging.staticTileEntityCandidates)
            {
                TileEntity *te = candidate.tileEntity;
                if (te == nullptr || te->isInvalid())
                    continue;

                // Meshing is incremental. Revalidate ownership immediately before
                // dereferencing/rendering a candidate so a chest/sign removed while
                // this section was being built can never leave a stale pointer here.
                TileEntity *current = worldObj->getBlockTileEntity(candidate.x, candidate.y, candidate.z);
                if (!pcLegacyStaticTileEntityCandidateIsCurrent(candidate, current))
                    continue;

                const int_t brightness = worldObj->getLightBrightnessForSkyBlocks(
                    candidate.x, candidate.y, candidate.z, 0);
                OpenGlHelper::setLightmapTextureCoords(OpenGlHelper::lightmapTexUnit,
                    static_cast<float>(brightness % 65536), static_cast<float>(brightness / 65536));
                renderColor4f(1.0f, 1.0f, 1.0f, 1.0f);

                const bool drew = TileEntityRenderer::instance.renderStaticPart(te,
                    static_cast<double>(candidate.x - posX),
                    static_cast<double>(candidate.y - posY),
                    static_cast<double>(candidate.z - posZ));
                if (!drew)
                    continue;

                staticTileDrawn = true;
                if (std::find(rebuiltStaticTileEntityRenderers.begin(), rebuiltStaticTileEntityRenderers.end(), te) ==
                    rebuiltStaticTileEntityRenderers.end())
                {
                    rebuiltStaticTileEntityRenderers.push_back(te);
                }
            }

            // A static chest/sign binds its own texture inside the display list.
            // Restore the terrain atlas so the next batched section starts from
            // the same texture state as before this optimization existed.
            renderBindTexture(ConnectedTextures::getTerrainTextureId());
            renderPopMatrix();
        }
#endif

        renderEndDisplayList();
        _skipRenderPass[pass] = !(compactDrawn || baseDrawn || extraDrawn || staticTileDrawn);
    }

    const std::vector<TileEntity *> &rebuiltTileEntityRenderers = staging.tileEntityRenderers;
    for (TileEntity *te : rebuiltTileEntityRenderers)
    {
        if (std::find(tileEntityRenderers.begin(), tileEntityRenderers.end(), te) == tileEntityRenderers.end())
            pushUniqueTileEntityRef(tileEntities, te);
    }
    for (TileEntity *te : tileEntityRenderers)
    {
        if (std::find(rebuiltTileEntityRenderers.begin(), rebuiltTileEntityRenderers.end(), te) == rebuiltTileEntityRenderers.end())
            eraseAllTileEntityRefs(tileEntities, te);
    }

#if PC_LEGACY_STATIC_TILE_ENTITY_MESH && PLATFORM_PC_LEGACY
    for (TileEntity *te : pcLegacyStaticTileEntityRenderers)
        pcLegacyStaticTileEntityUnpublish(te, this);
    pcLegacyStaticTileEntityRenderers = rebuiltStaticTileEntityRenderers;
    for (TileEntity *te : pcLegacyStaticTileEntityRenderers)
        pcLegacyStaticTileEntityPublish(te, this);
#else
    pcLegacyStaticTileEntityRenderers.clear();
#endif

    tileEntityRenderers = rebuiltTileEntityRenderers;
    if (staging.sectionCache.has_value())
    {
        for (int_t face = 0; face < 6; ++face)
            pcLegacyPublishedVisibility[face] = staging.sectionCache->getVisibleFacesFrom(face);
    }
    isChunkLit = pcLegacyBuildChunkLit;
    isInitialized = true;
    const bool dirtyDuringBuild = pcLegacyBuildDirtyDuringBuild;
    needsUpdate = dirtyDuringBuild;
    ++chunksUpdated;

    pcLegacyResetBuildState();
    pcLegacyStepDidWork = true;
}

bool WorldRenderer::pcLegacyBuildRendererStep(int_t blockBudget)
{
    pcLegacyStepDidWork = false;
    const std::uint64_t stepStartUs = PlatformCompat::getMonotonicMicros();

    if (worldObj == nullptr)
    {
        pcLegacyResetBuildState();
        needsUpdate = false;
        return true;
    }

    const int_t x0 = posX;
    const int_t y0 = posY;
    const int_t z0 = posZ;
    const int_t x1 = posX + sizeWidth;
    const int_t y1 = posY + sizeHeight;
    const int_t z1 = posZ + sizeDepth;

    unsigned int sourceAvailability = pcLegacySourceAvailability(worldObj, x0, z0, x1, z1);
    if (pcLegacyBuildActive && pcLegacyBuildSourceAvailabilityValid &&
        pcLegacyBuildSourceAvailability != sourceAvailability)
    {
        pcLegacyResetBuildState();
        return false;
    }

    const bool centralSectionEmpty = !pcLegacyBuildActive &&
        pcLegacyCentralSectionIsEmpty(worldObj, x0, y0, z0);

    const int_t ccx0 = JavaArithmetic::intShr(x0 - 1, 4);
    const int_t ccx1 = JavaArithmetic::intShr(x1 + 1, 4);
    const int_t ccz0 = JavaArithmetic::intShr(z0 - 1, 4);
    const int_t ccz1 = JavaArithmetic::intShr(z1 + 1, 4);
    int_t requestedDependencies = 0;
    unsigned int sourceBit = 1u;
    for (int_t ccx = ccx0; ccx <= ccx1 && !centralSectionEmpty; ++ccx)
    {
        for (int_t ccz = ccz0; ccz <= ccz1; ++ccz, sourceBit <<= 1)
        {
            if ((sourceAvailability & sourceBit) != 0u)
                continue;

            worldObj->getChunkFromChunkCoords(ccx, ccz);
            ++requestedDependencies;
            pcLegacyStepDidWork = true;
            if (requestedDependencies >= PC_LEGACY_RENDERER_DEPENDENCY_REQUESTS_PER_STEP)
                return false;
        }
    }
    if (requestedDependencies > 0)
        return false;

    if (!pcLegacyBuildActive)
    {
        if (!pcLegacyBeginBuildState())
            return false;
        pcLegacyBuildSourceAvailability = sourceAvailability;
        pcLegacyBuildSourceAvailabilityValid = true;
        pcLegacyStepDidWork = true;
        if (pcLegacyStepTimeExpired(stepStartUs))
            return false;
    }

    PcLegacyTerrainStaging *staging = pcLegacyTerrainStagingGet(this);
    if (staging == nullptr || !staging->sectionCache.has_value())
    {
        pcLegacyResetBuildState();
        return false;
    }

    PcLegacySectionCache &sectionCache = *staging->sectionCache;
    pcLegacyBuildHasPass1 = sectionCache.getRenderWorkCount(1) > 0;
    if (!sectionCache.hasRenderWork())
    {
        pcLegacyPublishBuild(*staging);
        return true;
    }

    const int_t totalBlocks = sizeWidth * sizeHeight * sizeDepth;
    if (blockBudget <= 0)
        blockBudget = totalBlocks;

    int_t processed = 0;
    while (pcLegacyBuildPass < 2)
    {
        if (pcLegacyBuildPass == 1 && !pcLegacyBuildHasPass1)
        {
            ++pcLegacyBuildPass;
            pcLegacyBuildCursor = 0;
            continue;
        }

        const int_t workCount = sectionCache.getRenderWorkCount(pcLegacyBuildPass);
        if (workCount <= 0)
        {
            ++pcLegacyBuildPass;
            pcLegacyBuildCursor = 0;
            continue;
        }

        sectionCache.beginStep();
        RenderBlocks renderblocks(&sectionCache);
        if (PLATFORM_COMPACT_TERRAIN_VERTICES)
            renderblocks.setPcLegacyCompactTerrainMesh(&staging->compactPassMeshes[pcLegacyBuildPass], posX, posY, posZ);
        Tessellator *tessellator = &Tessellator::instance;
        Chunk::isLit = false;
        tessellator->startDrawingQuads();
        tessellator->setTranslationD(-static_cast<double>(posX),
            -static_cast<double>(posY), -static_cast<double>(posZ));

        bool stepDrew = false;
        while (pcLegacyBuildCursor < workCount && processed < blockBudget)
        {
            if (processed > 0 &&
                (processed % PLATFORM_CHUNK_BUILD_TIME_CHECK_BLOCKS) == 0 &&
                pcLegacyStepTimeExpired(stepStartUs))
            {
                break;
            }

            const int_t cursor = static_cast<int_t>(sectionCache.getRenderWorkEntry(
                pcLegacyBuildPass, pcLegacyBuildCursor++));
            const int_t lx = cursor & 15;
            const int_t lz = (cursor >> 4) & 15;
            const int_t ly = (cursor >> 8) & 15;
            const int_t x = x0 + lx;
            const int_t y = y0 + ly;
            const int_t z = z0 + lz;
            ++processed;

            const int_t id = sectionCache.getBlockIdLocal(lx, ly, lz);
            if (id <= 0 || id >= Block::BLOCK_REGISTRY_SIZE)
                continue;

            Block *block = Block::blocksList[id];
            if (block == nullptr)
                continue;

            if (pcLegacyBuildPass == 0 && Block::isBlockContainer[id])
            {
                TileEntity *te = sectionCache.getBlockTileEntity(x, y, z);
                if (te != nullptr && TileEntityRenderer::instance.hasSpecialRenderer(te))
                {
                    if (std::find(staging->tileEntityRenderers.begin(), staging->tileEntityRenderers.end(), te) ==
                        staging->tileEntityRenderers.end())
                    {
                        staging->tileEntityRenderers.push_back(te);
                    }
#if PC_LEGACY_STATIC_TILE_ENTITY_MESH && PLATFORM_PC_LEGACY
                    if (TileEntityRenderer::instance.canRenderStaticPart(te))
                    {
                        const bool alreadyQueued = std::any_of(staging->staticTileEntityCandidates.begin(),
                            staging->staticTileEntityCandidates.end(),
                            [te](const PcLegacyStaticTileEntityCandidate &candidate) {
                                return candidate.tileEntity == te;
                            });
                        if (!alreadyQueued)
                        {
                            PcLegacyStaticTileEntityCandidate candidate{te, x, y, z};
                            if (pcLegacyStaticTileEntityCandidateInsideSection(candidate, posX, posY, posZ))
                                staging->staticTileEntityCandidates.push_back(candidate);
                        }
                    }
#endif
                }
            }

            const PcLegacyBlockRenderInfo &blockInfo = pcLegacyGetBlockRenderInfo(id);
            if (blockInfo.renderPass != pcLegacyBuildPass)
                continue;

            if (blockInfo.simpleOpaqueCube)
            {
                const std::uint8_t faceMask = sectionCache.getExposedFaceMaskLocal(lx, ly, lz);
                if (faceMask == 0)
                    continue;
                stepDrew |= renderblocks.renderSimpleOpaqueCubeLegacy(block, x, y, z, faceMask);
            }
            else
            {
                stepDrew |= renderblocks.renderBlockByRenderType(block, x, y, z);
            }
        }

        const bool textureGroupsCaptureOk = tessellator->captureTextureGroups(
            staging->extraTextureMeshes[pcLegacyBuildPass], true);
        const bool hadBaseVertices = tessellator->hasPendingVertices();
        const bool baseCaptureOk = tessellator->capture(staging->passMeshes[pcLegacyBuildPass], true);
        tessellator->setTranslationD(0.0, 0.0, 0.0);
        if (!textureGroupsCaptureOk || (hadBaseVertices && !baseCaptureOk))
        {
            pcLegacyResetBuildState();
            return false;
        }

        pcLegacyBuildChunkLit = pcLegacyBuildChunkLit || Chunk::isLit;
        pcLegacyStepDidWork = pcLegacyStepDidWork || processed > 0 || stepDrew;

        if (pcLegacyBuildCursor < workCount)
            return false;

        ++pcLegacyBuildPass;
        pcLegacyBuildCursor = 0;

        if (processed >= blockBudget || pcLegacyStepTimeExpired(stepStartUs))
            return false;
    }

    pcLegacyPublishBuild(*staging);
    return true;
}

#endif
