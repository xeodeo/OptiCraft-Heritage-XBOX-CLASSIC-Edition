#pragma once
#include "ItemMap.h"
class ItemEmptyMap : public ItemMap { public: explicit ItemEmptyMap(int id) : ItemMap(id) {} ItemStack *onItemRightClick(ItemStack *stack, World *world, EntityPlayer *player) override; };
