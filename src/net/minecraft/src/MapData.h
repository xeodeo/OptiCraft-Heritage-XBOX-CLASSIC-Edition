#pragma once

#include "MapDataBase.h"
#include "java/Type.h"
#include <vector>
#include <map>
#include <string>
#include <utility>

class EntityPlayer;
class ItemStack;
class MapInfo;
class MapCoord;
class NBTTagCompound;
class World;

// net.minecraft.src.MapData
class MapData : public MapDataBase
{
public:
	MapData(const std::string &s);
	~MapData() override;

	void readFromNBT(NBTTagCompound *nbttagcompound) override;
	void writeToNBT(NBTTagCompound *nbttagcompound) override;

	// Java 1.5.2 names. updateVisiblePlayers replaces 1.2.5's updateMapData.
	void updateVisiblePlayers(EntityPlayer *entityplayer, ItemStack *itemstack);
	void setColumnDirty(int_t i, int_t j, int_t k);
	void updateMPMapData(const std::vector<byte_t> &data);
	void handleMapPacket(const std::vector<byte_t> &data) { updateMPMapData(data); }
	MapInfo *func_82568_a(EntityPlayer *entityplayer);

	int_t xCenter;          // xCenter
	int_t zCenter;          // zCenter
	byte_t dimension;       // dimension
	byte_t scale;           // scale
	byte_t colors[16384];   // colors
	std::vector<MapInfo*> playersArrayList;   // playersArrayList
	// Java LinkedHashMap<String, MapCoord>: insertion order, a put on an
	// existing key keeps its place.
	std::vector<std::pair<std::string, MapCoord*>> playersVisibleOnMap;

private:
	void func_82567_a(int_t iconType, World *world, const std::string &name, double x, double z, double rotation);
	void putVisible(const std::string &name, MapCoord *coord);
	void removeVisible(const std::string &name);
	void clearVisible();

	std::map<EntityPlayer*, MapInfo*> playersHashMap; // playersHashMap
};
