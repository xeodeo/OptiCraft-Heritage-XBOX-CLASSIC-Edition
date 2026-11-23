#include "ChocoCraftWorld.h"
#include "ChocoCraftItems.h"
#include "EntityChocobo.h"
#include "mods/ModManager.h"

#include "net/minecraft/src/Minecraft.h"
#include "net/minecraft/src/World.h"
#include "net/minecraft/src/WorldProvider.h"
#include "net/minecraft/src/BiomeGenBase.h"
#include "net/minecraft/src/Material.h"
#include "net/minecraft/src/MathHelper.h"
#include "net/minecraft/src/IChunkProvider.h"
#include "net/minecraft/src/BlockTallGrass.h"
#include "net/minecraft/src/WorldGenFlowers.h"
#include "net/minecraft/src/RenderEngine.h"
#include "net/minecraft/src/TextureFX.h"
#include "java/Arithmetic.h"
#include "java/Random.h"
#include "platform/Log.h"

#include <cmath>
#include <vector>

namespace
{
bool chocoEnabled()
{
    return ModManager::getInstance().isModEnabled("chococraft");
}

// ---------------------------------------------------------------------------
// Block textures. The engine draws world blocks from terrain.png tiles only, so the
// two ChocoCraft blocks with a texture of their own (strawBlock, gysahlGreenBlock) get
// a spare tile that this static TextureFX fills from the 4.1.5 PNG. It is uploaded
// once and again after a texture pack change (uploadTextureFxTile skips unchanged
// pixels), and scaled to the pack's tile size like the other dynamic tiles.
// ---------------------------------------------------------------------------
class ChocoCraftTileFX : public TextureFX
{
public:
    ChocoCraftTileFX(int_t tile, const char *path, int_t fallbackArgb)
        : TextureFX(tile)
        , m_path(path)
        , m_fallback(fallbackArgb)
    {
        tileImage = 0;
    }

    void onTexturePackChanged(RenderEngine *re) override { load(re); }

    void load(RenderEngine *re)
    {
        std::vector<int_t> px;
        int_t w = 0;
        int_t h = 0;
        bool ok = re != nullptr && re->readTextureImageDataWithSize(m_path, px, w, h) && w > 0 && h > 0;
        if (!ok && re != nullptr)
            ok = re->readDefaultTextureImageDataWithSize(m_path, px, w, h) && w > 0 && h > 0;
        for (int_t y = 0; y < 16; ++y)
        {
            for (int_t x = 0; x < 16; ++x)
            {
                int_t argb = m_fallback;
                if (ok)
                {
                    const int_t sx = x * w / 16;
                    const int_t sy = y * h / 16;
                    argb = px[static_cast<std::size_t>(sy) * static_cast<std::size_t>(w) + static_cast<std::size_t>(sx)];
                }
                const int_t i = (y * 16 + x) * 4;
                imageData[i + 0] = static_cast<byte_t>((argb >> 16) & 0xFF);
                imageData[i + 1] = static_cast<byte_t>((argb >> 8) & 0xFF);
                imageData[i + 2] = static_cast<byte_t>(argb & 0xFF);
                imageData[i + 3] = static_cast<byte_t>((argb >> 24) & 0xFF);
            }
        }
    }

private:
    const char *m_path;
    int_t m_fallback;
};

void populateHook(World *world, int_t chunkX, int_t chunkZ)
{
    ChocoCraftWorld::generateGysahls(world, chunkX, chunkZ);
}

int_t grassSeedHook(Random &random)
{
    return ChocoCraftWorld::grassSeed(random);
}

// ModChocoCraft.spawnBiomes
bool isSpawnBiome(const BiomeGenBase *b)
{
    if (b == nullptr)
        return false;
    return b == BiomeGenBase::extremeHills || b == BiomeGenBase::extremeHillsEdge ||
           b == BiomeGenBase::forest || b == BiomeGenBase::forestHills ||
           b == BiomeGenBase::jungle || b == BiomeGenBase::jungleHills ||
           b == BiomeGenBase::plains || b == BiomeGenBase::swampland ||
           b == BiomeGenBase::taiga || b == BiomeGenBase::taigaHills ||
           b == BiomeGenBase::iceMountains || b == BiomeGenBase::icePlains ||
           b == BiomeGenBase::hell;
}

bool isWorldHell(World *world)
{
    // ChocoboBiomeHelper.isWorldHell: the Nether (and the End).
    return world->worldProvider != nullptr && world->worldProvider->worldType != 0;
}

bool isNormalCubesAround(World *world, int_t x, int_t y, int_t z)
{
    for (int_t dx = x - 1; dx <= x + 1; ++dx)
        for (int_t dz = z - 1; dz <= z + 1; ++dz)
            if (!world->isBlockNormalCube(dx, y, dz))
                return false;
    return true;
}

bool canChocoboSpawnAtLocation(World *world, int_t x, int_t y, int_t z)
{
    if (!isNormalCubesAround(world, x, y - 1, z))
        return false;
    if (isNormalCubesAround(world, x, y, z))
        return false;
    Material *m = world->getBlockMaterial(x, y, z);
    if (m != nullptr && m->isLiquid())
        return false;
    if (isNormalCubesAround(world, x, y + 1, z))
        return false;
    return !isNormalCubesAround(world, x, y + 2, z);
}

int_t spawnHeight(World *world, int_t x, int_t z)
{
    if (!isWorldHell(world))
        return world->getHeightValue(x, z);
    // The original takes the height map here too, which in the Nether is the bedrock
    // roof; look for the first floor with room above instead, so the Purple flock
    // can actually appear in the caverns.
    for (int_t y = 32; y < 100; ++y)
        if (canChocoboSpawnAtLocation(world, x, y, z))
            return y;
    return 128;
}

bool isUntamedChocobo(Entity *e)
{
    EntityChocobo *c = dynamic_cast<EntityChocobo *>(e);
    return c != nullptr && !c->isDead && !c->isTamed();
}
} // namespace

namespace ChocoCraftWorld
{
void install(Minecraft *mc)
{
    IChunkProvider::modPopulateHook = &populateHook;
    BlockTallGrass::grassSeedHook = &grassSeedHook;

    static bool s_tilesRegistered = false;
    if (!s_tilesRegistered && mc != nullptr && mc->renderEngine != nullptr)
    {
        s_tilesRegistered = true;
        ChocoCraftTileFX *green = new ChocoCraftTileFX(ChocoCraftItems::TILE_GYSAHL_GREEN,
            "/chococraft/textures/blocks/gysahl_green.png", static_cast<int_t>(0xFF4FA83Cu));
        ChocoCraftTileFX *straw = new ChocoCraftTileFX(ChocoCraftItems::TILE_STRAW,
            "/chococraft/textures/blocks/chocoboStraw.png", static_cast<int_t>(0xFFD8B84Au));
        green->load(mc->renderEngine);
        straw->load(mc->renderEngine);
        mc->renderEngine->registerTextureFX(green);
        mc->renderEngine->registerTextureFX(straw);
    }
}

void generateGysahls(World *world, int_t chunkX, int_t chunkZ)
{
    if (world == nullptr || world->multiplayerWorld || !chocoEnabled())
        return;
    // world.provider.isSurfaceWorld()
    if (world->worldProvider == nullptr || world->worldProvider->worldType != 0)
        return;
    if (ChocoCraftItems::blockGysahlGreen == nullptr)
        return;

    // Forge GameRegistry.generateWorld seed: a Random of its own, never the
    // generator's, so the vanilla blocks of the chunk stay exactly as without the mod.
    const long_t worldSeed = world->getRandomSeed();
    Random fmlRandom(worldSeed);
    const long_t xSeed = JavaArithmetic::longShr(fmlRandom.nextLong(), 3); // nextLong() >> 2 + 1L
    const long_t zSeed = JavaArithmetic::longShr(fmlRandom.nextLong(), 3);
    const ulong_t chunkSeed =
        (static_cast<ulong_t>(xSeed) * static_cast<ulong_t>(static_cast<long_t>(chunkX)) +
         static_cast<ulong_t>(zSeed) * static_cast<ulong_t>(static_cast<long_t>(chunkZ))) ^
        static_cast<ulong_t>(worldSeed);
    fmlRandom.setSeed(JavaArithmetic::longFromBits(chunkSeed));

    // WorldGenGysahls: random.nextInt(1000) <= gysahlWorldGenRate (160).
    if (fmlRandom.nextInt(1000) > 160)
        return;
    // +8 like the vanilla flowers, so the patch stays in the chunks being populated
    // (the 1.7.10 original spills into neighbours and can generate them).
    const int_t x = chunkX * 16 + fmlRandom.nextInt(16) + 8;
    const int_t z = chunkZ * 16 + fmlRandom.nextInt(16) + 8;
    const int_t y = world->getHeightValue(x, z);
    WorldGenFlowers(ChocoCraftItems::BLOCK_GYSAHL_GREEN_ID).generate(world, fmlRandom, x, y, z);
}

int_t grassSeed(Random &random)
{
    if (!chocoEnabled() || ChocoCraftItems::gysahlSeeds == nullptr)
        return 0;
    return random.nextInt(13) >= 10 ? ChocoCraftItems::gysahlSeeds->shiftedIndex : 0;
}

void doChocoboSpawning(World *world, double posX, double posY, double posZ)
{
    (void)posY;
    if (world == nullptr || world->multiplayerWorld || !chocoEnabled())
        return;

    const int_t maxTries = 20;
    if (world->rand.nextInt(100) > 2) // spawnProbability
        return;

    const bool hell = isWorldHell(world);
    const int_t outer = hell ? 48 : 64;
    const int_t inner = 32;

    int_t dx = 0;
    int_t dz = 0;
    bool canSpawnHere = false;
    for (int_t i = 0; i < maxTries; ++i)
    {
        dx = world->rand.nextInt(outer) + inner;
        dz = world->rand.nextInt(outer) + inner;
        if (world->rand.nextBoolean())
            dx = -dx;
        if (world->rand.nextBoolean())
            dz = -dz;
        const int_t tx = MathHelper::floor_double(posX + dx);
        const int_t tz = MathHelper::floor_double(posZ + dz);
        if (!world->blockExists(tx, 64, tz))
            continue;
        canSpawnHere = hell || isSpawnBiome(world->getBiomeGenForCoords(tx, tz));
        // isOtherPlayerNear
        const int_t ty = world->getHeightValue(tx, tz);
        if (canSpawnHere && world->getClosestPlayer(tx, ty, tz, inner / 2) != nullptr)
            canSpawnHere = false;
        if (canSpawnHere)
            break;
    }
    if (!canSpawnHere)
        return;

    const int_t baseX = MathHelper::floor_double(posX + dx);
    const int_t baseZ = MathHelper::floor_double(posZ + dz);

    // countWildInChunkRadius (spawnLimitChunkRadius = 100 blocks) > spawnTotalMax (3),
    // and distanceToNextWild < spawnDistanceNextWild (300).
    const int_t minCX = (baseX - 100) >> 4, maxCX = (baseX + 100) >> 4;
    const int_t minCZ = (baseZ - 100) >> 4, maxCZ = (baseZ + 100) >> 4;
    int_t wildInChunks = 0;
    double nearestWild = 10000.0;
    for (Entity *e : world->loadedEntityList)
    {
        if (!isUntamedChocobo(e))
            continue;
        const int_t ecx = MathHelper::floor_double(e->posX) >> 4;
        const int_t ecz = MathHelper::floor_double(e->posZ) >> 4;
        if (ecx >= minCX && ecx <= maxCX && ecz >= minCZ && ecz <= maxCZ)
            ++wildInChunks;
        const double ddx = baseX - e->posX;
        const double ddz = baseZ - e->posZ;
        const double d = std::sqrt(ddx * ddx + ddz * ddz);
        if (d < nearestWild)
            nearestWild = d;
    }
    if (wildInChunks > 3)
        return;
    if (static_cast<int_t>(std::floor(nearestWild)) < 300)
        return;

    // Flock of spawnGroupMin (2) + nextInt(max - min) = 2..3, Yellow (Purple in the Nether).
    const int_t groupSize = 2 + world->rand.nextInt(2);
    int_t spawned = 0;
    for (int_t i = 0; i < maxTries; ++i)
    {
        const int_t x = static_cast<int_t>(posX) + dx + world->rand.nextInt(6);
        const int_t z = static_cast<int_t>(posZ) + dz + world->rand.nextInt(6);
        const int_t y = spawnHeight(world, x, z);
        const int_t yaw = world->rand.nextInt(360);
        if (!canChocoboSpawnAtLocation(world, x, y, z))
            continue;
        EntityChocobo *choco = new EntityChocobo(world);
        choco->setColor(hell ? ChocoboColor::Purple : ChocoboColor::Yellow);
        choco->setLocationAndAngles(static_cast<double>(x), static_cast<double>(y), static_cast<double>(z),
                                    static_cast<float>(yaw), 0.0f);
        if (!world->entityJoinedWorld(choco))
        {
            delete choco;
            continue;
        }
        if (++spawned >= groupSize)
            break;
    }
    if (spawned > 0)
        MC_LOG_INFO("mods", "ChocoCraft: %d wild %s chocobos at %d,%d\n", static_cast<int>(spawned),
                    hell ? "Purple" : "Yellow", static_cast<int>(baseX), static_cast<int>(baseZ));
}
} // namespace ChocoCraftWorld
