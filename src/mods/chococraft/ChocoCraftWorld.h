#pragma once

#include "java/Type.h"

class Minecraft;
class World;
class Random;

// ChocoCraft 4.1.5 world side: WorldGenGysahls (Gysahl bushes in new chunks), the
// tall grass Gysahl seed (ChocoCraftWorld.addGrassDrops), ChocoboSpawner (wild flocks)
// and the runtime textures of the two ChocoCraft blocks drawn from terrain.png.
namespace ChocoCraftWorld
{
    // Hooks into the chunk generators / tall grass and registers the block textures.
    void install(Minecraft *mc);

    // WorldGenGysahls.generate: own Random from the world seed and the chunk (the
    // Forge IWorldGenerator seed), so the vanilla terrain of a seed never changes.
    // Does nothing unless the ChocoCraft mod is on.
    void generateGysahls(World *world, int_t chunkX, int_t chunkZ);

    // Forge grass seed list: wheat seeds weight 10, Gysahl seeds weight 3.
    int_t grassSeed(Random &random);

    // ChocoboSpawner.doChocoboSpawning around one player (every 200 ticks).
    void doChocoboSpawning(World *world, double posX, double posY, double posZ);
}
