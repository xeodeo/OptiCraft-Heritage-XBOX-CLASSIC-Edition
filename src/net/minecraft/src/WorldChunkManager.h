#pragma once

#include <memory>
#include <vector>

#include "NoiseBuffer.h"
#include "platform/PlatformTuning.h"
#include "java/Type.h"

class BiomeCache;
class BiomeGenBase;
class ChunkCoordIntPair;
class ChunkPosition;
class GenLayer;
class NoiseGeneratorOctaves;
class Random;
class World;
class WorldType;

// net.minecraft.src.WorldChunkManager
class WorldChunkManager
{
public:
    WorldChunkManager();
    WorldChunkManager(long_t worldSeed, WorldType *worldType);
    explicit WorldChunkManager(World *world);
    virtual ~WorldChunkManager();

    virtual std::vector<BiomeGenBase *> &getBiomesToSpawnIn();
    virtual BiomeGenBase *getBiomeGenAtChunkCoord(ChunkCoordIntPair *chunkCoord);
    virtual BiomeGenBase *getBiomeGenAt(int_t x, int_t z);
    virtual double getTemperature(int_t x, int_t z);

    virtual BiomeNoiseBuffer &getRainfall(BiomeNoiseBuffer &values,
                                           int_t x, int_t z, int_t width, int_t height);
    virtual float getTemperatureAtHeight(float temperatureValue, int_t y);
    virtual BiomeNoiseBuffer &getTemperatures(BiomeNoiseBuffer &values,
                                               int_t x, int_t z, int_t width, int_t height);

    virtual std::vector<BiomeGenBase *> &getBiomesForGeneration(std::vector<BiomeGenBase *> &values,
                                                                 int_t x, int_t z, int_t width, int_t height);
    virtual std::vector<BiomeGenBase *> &loadBlockGeneratorData(std::vector<BiomeGenBase *> &values,
                                                                 int_t x, int_t z, int_t width, int_t height);
    virtual std::vector<BiomeGenBase *> &getBiomeGenAt(std::vector<BiomeGenBase *> &values,
                                                        int_t x, int_t z, int_t width, int_t height,
                                                        bool useCache);

    virtual bool areBiomesViable(int_t x, int_t z, int_t radius,
                                  const std::vector<BiomeGenBase *> &allowedBiomes);
    virtual ChunkPosition *findBiomePosition(int_t x, int_t z, int_t radius,
                                              const std::vector<BiomeGenBase *> &allowedBiomes,
                                              Random &random);
    virtual void cleanupCache();

    // Compatibility API used by Beta renderer color paths while those callers
    // are migrated to direct 1.2.5 biome temperature/rainfall lookups.
    virtual std::vector<BiomeGenBase *> &getBiomeBlock(int_t x, int_t z, int_t width, int_t height);

    bool isLimitedWorld() const { return limitedWorld; }
    void setLimitedWorld(bool limited) { limitedWorld = limited; if (limited && worldSizeType == 0) worldSizeType = 1; else if (!limited) worldSizeType = 0; }
    int_t getWorldSizeType() const { return worldSizeType; }
    void setWorldSizeType(int_t type) { worldSizeType = type; limitedWorld = (type != 0); }

    BiomeNoiseBuffer temperature;
    BiomeNoiseBuffer humidity;

protected:
    bool limitedWorld = false;
    int_t worldSizeType = 0;
    static int_t checkedBiomeAreaCount(int_t width, int_t height);

    // Single point where a biome id area is produced. Every biome-derived value in
    // this class -- id, temperature, rainfall -- reads through one of these two,
    // so a replacement source only has to satisfy them. Both return GenLayer's
    // layout: index = j * width + i is the biome at (x + i, z + j).
    //
    // The coarse form backs the callers that used genBiomes (the pre-Voronoi
    // layer): generation-time biome lookups and the structure/spawn searches.
    const std::vector<int_t> &biomeIdArea(int_t x, int_t z, int_t width, int_t height);
    const std::vector<int_t> &coarseBiomeIdArea(int_t x, int_t z, int_t width, int_t height);

#if PLATFORM_FAST_BIOME_SOURCE
    // WorldChunkManagerFast.cpp
    void initFastBiomeSource(long_t worldSeed);
    const std::vector<int_t> &fastBiomeIdArea(int_t x, int_t z, int_t width, int_t height);

    std::unique_ptr<NoiseGeneratorOctaves> fastContinentNoise;
    std::unique_ptr<NoiseGeneratorOctaves> fastTemperatureNoise;
    std::unique_ptr<NoiseGeneratorOctaves> fastHumidityNoise;
    TerrainNoiseBuffer fastContinentField;
    TerrainNoiseBuffer fastTemperatureField;
    TerrainNoiseBuffer fastHumidityField;
#endif

    std::vector<int_t> fastBiomeIds;
    std::vector<int_t> fastCoarseBiomeIds;

    std::shared_ptr<GenLayer> genBiomes;
    std::shared_ptr<GenLayer> biomeIndexLayer;
    std::unique_ptr<BiomeCache> biomeCache125;
    std::vector<BiomeGenBase *> biomesToSpawnIn;
    std::vector<BiomeGenBase *> biomeBuffer;
};
