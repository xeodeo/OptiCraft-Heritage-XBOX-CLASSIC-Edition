#include "EntityMinecartEmpty.h"
#include "EntityMinecartChest.h"
#include "EntityMinecartFurnace.h"
#include "EntityMinecartTNT.h"
#include "World.h"

EntityMinecartEmpty::EntityMinecartEmpty(World *w) : EntityMinecart(w) { minecartType = 0; }
EntityMinecartEmpty::EntityMinecartEmpty(World *w,double x,double y,double z) : EntityMinecart(w,x,y,z,0) {}
EntityMinecartChest::EntityMinecartChest(World *w) : EntityMinecart(w) { minecartType = 1; }
EntityMinecartChest::EntityMinecartChest(World *w,double x,double y,double z) : EntityMinecart(w,x,y,z,1) {}
EntityMinecartFurnace::EntityMinecartFurnace(World *w) : EntityMinecart(w) { minecartType = 2; }
EntityMinecartFurnace::EntityMinecartFurnace(World *w,double x,double y,double z) : EntityMinecart(w,x,y,z,2) {}
EntityMinecartTNT::EntityMinecartTNT(World *w) : EntityMinecart(w) { minecartType = 3; }
EntityMinecartTNT::EntityMinecartTNT(World *w,double x,double y,double z) : EntityMinecart(w,x,y,z,3) {}
void EntityMinecartTNT::onActivatorRailPass(int_t, int_t, int_t, bool powered) { if (powered && fuse < 0) fuse = 80; }
void EntityMinecartTNT::onUpdate() { EntityMinecart::onUpdate(); if (fuse > 0) { --fuse; worldObj->spawnParticle("smoke",posX,posY+0.5,posZ,0,0,0); } else if (fuse == 0) { worldObj->createExplosion(this,posX,posY,posZ,4.0f); setEntityDead(); } }
