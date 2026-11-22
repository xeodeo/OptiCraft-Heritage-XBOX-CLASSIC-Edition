#include "ChocoCraftMod.h"
#include "ChocoCraftItems.h"
#include "EntityChocobo.h"

#include "net/minecraft/src/Minecraft.h"
#include "net/minecraft/src/World.h"
#include "net/minecraft/src/WorldProvider.h"
#include "net/minecraft/src/Block.h"
#include "net/minecraft/src/BlockGrass.h"
#include "net/minecraft/src/BiomeGenBase.h"
#include "net/minecraft/src/EntityPlayerSP.h"
#include "platform/Log.h"

#include <cmath>

namespace
{
ChocoCraftMod *s_chocoInstance = nullptr;

bool isChocoboBiome(const BiomeGenBase *biome)
{
    if (biome == nullptr)
        return false;
    const std::string &n = biome->biomeName;
    return n == "Plains" || n == "Forest" || n == "ForestHills" ||
           n == "Extreme Hills" || n == "Taiga" || n == "Swampland" ||
           n == "Jungle" || n == "JungleHills";
}
} // namespace

ChocoCraftMod::ChocoCraftMod()
    : m_mc(nullptr)
    , m_enabled(false)
    , m_spawnTimer(120)
    , m_plantTimer(80)
{
    s_chocoInstance = this;
}

ChocoCraftMod *ChocoCraftMod::getInstance()
{
    return s_chocoInstance;
}

void ChocoCraftMod::onInit(Minecraft *mc)
{
    m_mc = mc;
    s_chocoInstance = this;
    ChocoCraftItems::registerAll();
}

void ChocoCraftMod::trySpawnWildChocobo(World *world, EntityPlayer *player)
{
    if (world == nullptr || player == nullptr || world->multiplayerWorld)
        return;
    if (world->worldProvider != nullptr && world->worldProvider->worldType != 0)
        return;

    // Cap nearby chocobos to keep performance rock-solid (max 6 within 64 blocks)
    int_t nearbyCount = 0;
    for (Entity *ent : world->loadedEntityList)
    {
        if (ent != nullptr && !ent->isDead && dynamic_cast<EntityChocobo *>(ent) != nullptr)
        {
            if (ent->getDistanceSqToEntity(player) < 64.0 * 64.0)
                nearbyCount++;
        }
    }
    if (nearbyCount >= 6)
        return;

    for (int attempt = 0; attempt < 8; ++attempt)
    {
        const float angle = world->rand.nextFloat() * 6.283185f;
        const float dist = 18.0f + world->rand.nextFloat() * 24.0f;
        const int_t sx = static_cast<int_t>(std::floor(player->posX + std::cos(angle) * dist));
        const int_t sz = static_cast<int_t>(std::floor(player->posZ + std::sin(angle) * dist));
        if (!world->blockExists(sx, 64, sz))
            continue;
        if (!isChocoboBiome(world->getBiomeGenForCoords(sx, sz)))
            continue;

        const int_t sy = world->getTopSolidOrLiquidBlock(sx, sz);
        if (sy < 60 || sy > 120)
            continue;
        if (world->getBlockId(sx, sy - 1, sz) != Block::grass->blockID)
            continue;
        if (!world->isAirBlock(sx, sy, sz) || !world->isAirBlock(sx, sy + 1, sz))
            continue;

        EntityChocobo *choco = new EntityChocobo(world);
        choco->setColor(ChocoboColor::Yellow);
        choco->setLocationAndAngles(sx + 0.5, static_cast<double>(sy), sz + 0.5, world->rand.nextFloat() * 360.0f, 0.0f);
        if (world->entityJoinedWorld(choco))
        {
            MC_LOG_INFO("mods", "Spawned wild Yellow Chocobo at (%d,%d,%d)\n",
                        static_cast<int>(sx), static_cast<int>(sy), static_cast<int>(sz));
            return;
        }
        delete choco;
    }
}

void ChocoCraftMod::trySpawnWildGysahl(World *world, EntityPlayer *player)
{
    if (world == nullptr || player == nullptr || world->multiplayerWorld)
        return;
    if (world->worldProvider != nullptr && world->worldProvider->worldType != 0)
        return;

    const float angle = world->rand.nextFloat() * 6.283185f;
    const float dist = 12.0f + world->rand.nextFloat() * 26.0f;
    const int_t sx = static_cast<int_t>(std::floor(player->posX + std::cos(angle) * dist));
    const int_t sz = static_cast<int_t>(std::floor(player->posZ + std::sin(angle) * dist));
    if (!world->blockExists(sx, 64, sz))
        return;
    if (!isChocoboBiome(world->getBiomeGenForCoords(sx, sz)))
        return;

    const int_t sy = world->getTopSolidOrLiquidBlock(sx, sz);
    if (sy < 60 || sy > 120)
        return;
    if (world->getBlockId(sx, sy - 1, sz) != Block::grass->blockID)
        return;
    if (!world->isAirBlock(sx, sy, sz))
        return;

    // Avoid crowding: ensure no Gysahl already within 10 blocks
    for (int_t dx = -8; dx <= 8; dx += 2)
    {
        for (int_t dz = -8; dz <= 8; dz += 2)
        {
            if (world->getBlockId(sx + dx, sy, sz + dz) == ChocoCraftItems::BLOCK_GYSAHL_ID)
                return;
        }
    }

    world->setBlockAndMetadataWithNotify(sx, sy, sz, ChocoCraftItems::BLOCK_GYSAHL_ID, 7);
}

void ChocoCraftMod::onTick()
{
    if (!m_enabled || m_mc == nullptr || m_mc->theWorld == nullptr || m_mc->theWorld->multiplayerWorld)
        return;

    EntityPlayer *player = m_mc->thePlayerOne ? m_mc->thePlayerOne : m_mc->thePlayer;
    if (player == nullptr)
        return;

    if (--m_spawnTimer <= 0)
    {
        trySpawnWildChocobo(m_mc->theWorld, player);
        m_spawnTimer = 140; // ~7 seconds
    }

    if (--m_plantTimer <= 0)
    {
        trySpawnWildGysahl(m_mc->theWorld, player);
        m_plantTimer = 200; // ~10 seconds
    }
}
