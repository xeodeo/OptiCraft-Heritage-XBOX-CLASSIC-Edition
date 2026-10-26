#pragma once
#include "ItemMap.h"
class ItemEmptyMap : public ItemMap {
public:
    explicit ItemEmptyMap(int id) : ItemMap(id) {}
    ItemStack *onItemRightClick(ItemStack *stack, World *world, EntityPlayer *player) override;
    int createMapId(World *world) const;
    std::string mapKey(int id) const;
    int mapSize() const;
    MapData *createMapData(const std::string &key, World *world, EntityPlayer *player) const;
    ItemStack *makeMapStack(int id) const;
    bool giveMapToPlayer(EntityPlayer *player, ItemStack *map) const;
};
