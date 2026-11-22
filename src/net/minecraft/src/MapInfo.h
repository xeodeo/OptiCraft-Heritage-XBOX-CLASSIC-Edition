#pragma once

#include "java/Type.h"

class EntityPlayer;
class MapData;

// net.minecraft.src.MapInfo
class MapInfo
{
public:
	MapInfo(MapData *mapdata, EntityPlayer *entityplayer);

	EntityPlayer *entityplayerObj;
	int_t minVisibleY[128]; // minVisibleY
	int_t maxVisibleY[128]; // maxVisibleY
	int_t hasSkyLight;
	int_t isHellWorld;
	int_t field_82569_d; // Java 1.5.2: per-player column counter for ItemMap.updateMapData
	MapData *mapDataObj;
};
