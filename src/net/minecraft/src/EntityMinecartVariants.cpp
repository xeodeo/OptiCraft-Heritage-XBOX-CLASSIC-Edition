#include "EntityMinecartEmpty.h"
#include "EntityMinecartChest.h"
#include "EntityMinecartFurnace.h"
#include "World.h"

EntityMinecartEmpty::EntityMinecartEmpty(World *w) : EntityMinecart(w) { minecartType = 0; }
EntityMinecartEmpty::EntityMinecartEmpty(World *w, double x, double y, double z) : EntityMinecart(w, x, y, z, 0) {}

void EntityMinecartEmpty::onActivatorRailPass(int_t x, int_t y, int_t z, bool powered)
{
    (void)x; (void)y; (void)z;
    if (powered && riddenByEntity != nullptr)
    {
        riddenByEntity->mountEntity(nullptr);
    }
}

EntityMinecartChest::EntityMinecartChest(World *w) : EntityMinecart(w) { minecartType = 1; }
EntityMinecartChest::EntityMinecartChest(World *w, double x, double y, double z) : EntityMinecart(w, x, y, z, 1) {}

EntityMinecartFurnace::EntityMinecartFurnace(World *w) : EntityMinecart(w) { minecartType = 2; }
EntityMinecartFurnace::EntityMinecartFurnace(World *w, double x, double y, double z) : EntityMinecart(w, x, y, z, 2) {}
