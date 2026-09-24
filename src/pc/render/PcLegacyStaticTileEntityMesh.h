#pragma once

#include "platform/PlatformConfig.h"

#if PLATFORM_INCREMENTAL_TERRAIN_BUILD

class TileEntity;
class WorldRenderer;

bool pcLegacyStaticTileEntityIsBaked(TileEntity *tileEntity);
bool pcLegacyStaticTileEntityIsVisible(TileEntity *tileEntity);
void pcLegacyStaticTileEntityPublish(TileEntity *tileEntity, WorldRenderer *owner);
void pcLegacyStaticTileEntityUnpublish(TileEntity *tileEntity, WorldRenderer *owner);
void pcLegacyStaticTileEntityClearOwner(WorldRenderer *owner);

#endif
