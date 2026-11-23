#include "ChocoCraftMod.h"
#include "ChocoCraftItems.h"
#include "ChocoCraftWorld.h"

#include "net/minecraft/src/Minecraft.h"
#include "net/minecraft/src/World.h"
#include "net/minecraft/src/EntityPlayer.h"

#include <vector>

namespace
{
ChocoCraftMod *s_chocoInstance = nullptr;
} // namespace

ChocoCraftMod::ChocoCraftMod()
    : m_mc(nullptr)
    , m_enabled(false)
    , m_spawnTimer(0)
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
    // Gysahl world generation, Gysahl seeds from tall grass and the block textures.
    // Every hook checks that the mod is on before doing anything.
    ChocoCraftWorld::install(mc);
}

void ChocoCraftMod::onTick()
{
    if (!m_enabled || m_mc == nullptr || m_mc->theWorld == nullptr || m_mc->theWorld->multiplayerWorld ||
        m_mc->isGamePaused)
        return;

    // ServerSpawnTickHandler: every spawnTimeDelay (200) ticks, ChocoboSpawner around
    // each player in the world (both players in split screen).
    if (m_spawnTimer++ < 200)
        return;
    m_spawnTimer = 0;

    World *world = m_mc->theWorld;
    const std::vector<EntityPlayer *> players = world->playerEntities;
    for (EntityPlayer *player : players)
    {
        if (player != nullptr && !player->isDead)
            ChocoCraftWorld::doChocoboSpawning(world, player->posX, player->posY, player->posZ);
    }
}
