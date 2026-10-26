#pragma once
#include "BiomeGenBase.h"

class WorldGenerator;

// net.minecraft.src.BiomeGenHills
class BiomeGenHills : public BiomeGenBase
{
public:
    BiomeGenHills();
    ~BiomeGenHills() override;

    void decorate(World *world, Random &random, int_t chunkX, int_t chunkZ) override;

private:
    WorldGenerator *theWorldGenerator;
};
