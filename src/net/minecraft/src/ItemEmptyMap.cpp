#include "ItemEmptyMap.h"
#include "Item.h"
#include "ItemStack.h"
#include "EntityPlayer.h"
#include "InventoryPlayer.h"
#include "MapData.h"
#include "World.h"
#include "WorldInfo.h"
#include "WorldProvider.h"
ItemStack *ItemEmptyMap::onItemRightClick(ItemStack *stack, World *world, EntityPlayer *player) {
    int id=world->getUniqueDataId("map"); ItemStack *map=new ItemStack(Item::mapItem,1,id); std::string key="map_"+std::to_string(id); MapData *data=new MapData(key); data->scale=0; int size=128; data->xCenter=(int)(player->posX/size)*size; data->zCenter=(int)(player->posZ/size)*size; data->dimension=(byte_t)world->worldProvider->worldType; data->markDirty(); world->setItemData(key,data); --stack->stackSize; if(stack->stackSize<=0){delete stack;return map;} if(!player->inventory->addItemStackToInventory(map->copy())) player->dropPlayerItem(map); delete map; return stack;
}
int ItemEmptyMap::createMapId(World *world) const { return world ? world->getUniqueDataId("map") : 0; }
std::string ItemEmptyMap::mapKey(int id) const { return "map_" + std::to_string(id); }
int ItemEmptyMap::mapSize() const { return 128; }
MapData *ItemEmptyMap::createMapData(const std::string &key, World *world, EntityPlayer *player) const {
    MapData *data = new MapData(key); data->scale = 0;
    if (player) { data->xCenter = (int)(player->posX / mapSize()) * mapSize(); data->zCenter = (int)(player->posZ / mapSize()) * mapSize(); }
    data->dimension = world && world->worldProvider ? (byte_t)world->worldProvider->worldType : 0; data->markDirty(); return data;
}
ItemStack *ItemEmptyMap::makeMapStack(int id) const { return new ItemStack(Item::mapItem, 1, id); }
bool ItemEmptyMap::giveMapToPlayer(EntityPlayer *player, ItemStack *map) const { return player && map && player->inventory->addItemStackToInventory(map->copy()); }
