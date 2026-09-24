#include "pc/render/PcLegacyStaticTileEntityMesh.h"

#if PLATFORM_INCREMENTAL_TERRAIN_BUILD

#include <unordered_map>
#include <vector>

#include "net/minecraft/src/TileEntity.h"
#include "net/minecraft/src/WorldRenderer.h"

namespace
{
    std::unordered_map<TileEntity *, WorldRenderer *> g_bakedOwners;
}

bool pcLegacyStaticTileEntityIsBaked(TileEntity *tileEntity)
{
    return tileEntity != nullptr && g_bakedOwners.find(tileEntity) != g_bakedOwners.end();
}

bool pcLegacyStaticTileEntityIsVisible(TileEntity *tileEntity)
{
    auto it = g_bakedOwners.find(tileEntity);
    if (it == g_bakedOwners.end() || it->second == nullptr)
        return true;
    WorldRenderer *owner = it->second;
    return owner->isInFrustum && owner->pcLegacyCpuVisible;
}

void pcLegacyStaticTileEntityPublish(TileEntity *tileEntity, WorldRenderer *owner)
{
    if (tileEntity == nullptr || owner == nullptr)
        return;
    g_bakedOwners[tileEntity] = owner;
}

void pcLegacyStaticTileEntityUnpublish(TileEntity *tileEntity, WorldRenderer *owner)
{
    if (tileEntity == nullptr)
        return;
    auto it = g_bakedOwners.find(tileEntity);
    if (it != g_bakedOwners.end() && (owner == nullptr || it->second == owner))
        g_bakedOwners.erase(it);
}

void pcLegacyStaticTileEntityClearOwner(WorldRenderer *owner)
{
    if (owner == nullptr || g_bakedOwners.empty())
        return;
    for (auto it = g_bakedOwners.begin(); it != g_bakedOwners.end(); )
    {
        if (it->second == owner)
            it = g_bakedOwners.erase(it);
        else
            ++it;
    }
}

#endif
