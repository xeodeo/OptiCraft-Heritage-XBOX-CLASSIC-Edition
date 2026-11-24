
#if XBOX_AUTOPILOT
// ===== PROBE122 (temporary test driver, removed before commit) =====
#include "net/minecraft/src/EntityList.h"
#include "net/minecraft/src/WorldSettings.h"
#include "net/minecraft/src/WorldType.h"
#include "net/minecraft/src/PlayerControllerSP.h"
#include "platform/Log.h"
bool g_probeForceRemote = false;
namespace
{
int s_probeMode = -1; // -1 unknown, 0 off, 1 on
bool s_probeStarted = false;
int s_probeT = 0;
int s_probePhaseT = 0;
int s_probePhase = 0;
bool s_probeOrigLegacy = true;
double s_px, s_py, s_pz;
int s_count0 = 0;
}
#define PLOG(...) MC_LOG_INFO("mods", "[PROBE] " __VA_ARGS__)

static int probeCountBlocks(World *w, int cx, int cy, int cz)
{
    int n = 0;
    for (int dx = -4; dx <= 4; ++dx)
        for (int dy = 0; dy <= 4; ++dy)
            for (int dz = -4; dz <= 4; ++dz)
                if (w->getBlockId(cx + dx, cy + dy, cz + dz) != 0)
                    ++n;
    return n;
}

static int probeCountMobs(World *w, EntityPlayerSP *p, double r)
{
    int n = 0;
    for (Entity *e : w->loadedEntityList)
        if (e != nullptr && e != p && e->isLiving() && !e->isPlayer() && !e->isDead && p->getDistanceSqToEntity(e) < r * r)
            ++n;
    return n;
}

static void probeSpawn(World *w, EntityPlayerSP *p, const char *name, double ox, double oz)
{
    Entity *e = EntityList::createEntityByName(name, w);
    if (e == nullptr) { PLOG("spawn %s failed\n", name); return; }
    const int x = (int)std::floor(p->posX + ox), z = (int)std::floor(p->posZ + oz);
    const int y = w->getTopSolidOrLiquidBlock(x, z);
    e->setLocationAndAngles(x + 0.5, y + 0.1, z + 0.5, 0.0f, 0.0f);
    w->spawnEntityInWorld(e);
}

void AdminModeMod::probeTick()
{
    Minecraft *mc = m_mc;
    if (s_probeMode < 0)
    {
        s_probeMode = 0;
        if (FILE *f = std::fopen("D:\\probe122.txt", "r")) { s_probeMode = 1; std::fclose(f); }
    }
    if (s_probeMode != 1)
        return;
    if (mc->theWorld == nullptr)
    {
        if (!s_probeStarted && mc->currentScreen != nullptr && ++s_probeT > 60)
        {
            s_probeStarted = true;
            PLOG("starting world\n");
            delete mc->playerController;
            mc->playerController = new PlayerControllerSP(mc);
            WorldSettings settings(20261008LL, 0, true, false, WorldType::DEFAULT, 0);
            mc->startWorld("probe122", "probe122", &settings);
            mc->displayGuiScreen(nullptr);
        }
        return;
    }
    EntityPlayerSP *p = mc->thePlayer;
    if (p == nullptr)
        return;
    World *w = mc->theWorld;
    ++s_probePhaseT;
    const bool menuOpen = dynamic_cast<GuiAdminMenu *>(mc->currentScreen) != nullptr;
    auto next = [&](const char *what) { PLOG("phase %d done: %s\n", s_probePhase, what); ++s_probePhase; s_probePhaseT = 0; };

    switch (s_probePhase)
    {
    case 0: // wait for the world to settle
        if (s_probePhaseT == 1)
        {
            ModManager::getInstance().setModEnabled("adminmode", true);
            allOff();
            s_probeOrigLegacy = mc->gameSettings->legacyUI;
            PLOG("world ready legacyUI=%d pos=%.1f %.1f %.1f\n", s_probeOrigLegacy ? 1 : 0, p->posX, p->posY, p->posZ);
        }
        if (s_probePhaseT > 200 && p->onGround)
            next("settle");
        break;
    case 1: // wait for the Back+X combo (autopilot) to open the menu
        if (menuOpen)
        {
            PLOG("COMBO Back+X opened GuiAdminMenu after %d ticks\n", s_probePhaseT);
            mc->displayGuiScreen(nullptr);
            next("combo ok");
        }
        else if (s_probePhaseT > 1600)
        {
            PLOG("COMBO not seen\n");
            next("combo timeout");
        }
        break;
    case 2: // legacy menu shot
        if (s_probePhaseT == 1)
        {
            mc->gameSettings->legacyUI = true;
            mc->displayGuiScreen(new GuiAdminMenu(nullptr, 0));
            PLOG("SHOT menu legacy\n");
        }
        if (s_probePhaseT > 160) { mc->displayGuiScreen(nullptr); next("menu legacy"); }
        break;
    case 3: // java menu shot
        if (s_probePhaseT == 1)
        {
            mc->gameSettings->legacyUI = false;
            mc->displayGuiScreen(new GuiAdminMenu(nullptr, 0));
            PLOG("SHOT menu java\n");
        }
        if (s_probePhaseT > 160)
        {
            mc->displayGuiScreen(nullptr);
            mc->gameSettings->legacyUI = s_probeOrigLegacy;
            next("menu java");
        }
        break;
    case 4: // visual modules + movement
        if (menuOpen) mc->displayGuiScreen(nullptr);
        if (s_probePhaseT == 1)
        {
            probeSpawn(w, p, "Pig", 5, 3);
            probeSpawn(w, p, "Pig", -6, 4);
            probeSpawn(w, p, "Zombie", 3, 9);
            probeSpawn(w, p, "Cow", -4, -8);
            w->getWorldInfo()->setRaining(true);
            w->setRainStrength(1.0f);
            w->setWorldTime(w->getWorldTime() - (w->getWorldTime() % 24000) + 18000);
            setModuleOption(BlockESP, 3); // coal
            const int on[] = {Fly, NoFall, FullBright, ESP, Tracers, ChestFinder, BlockESP, Day, Weather, Step, Sprint, Spider};
            for (int m : on) setModuleState(m, true);
            setModuleOption(ESP, 2);
            setModuleOption(Tracers, 2);
            PLOG("visual on\n");
        }
        if (s_probePhaseT == 120)
        {
            PLOG("SHOT esp/fullbright flying=%d allowFly=%d step=%.2f time=%lld rain=%.2f raining=%d blockHits=%u mobs16=%d\n",
                 p->capabilities.isFlying ? 1 : 0, p->capabilities.allowFlying ? 1 : 0, p->stepHeight,
                 (long long)(w->getWorldTime() % 24000), w->getRainStrength(1.0f), w->getWorldInfo()->isRaining() ? 1 : 0,
                 (unsigned)m_blockHits.size(), probeCountMobs(w, p, 16.0));
        }
        if (s_probePhaseT > 240) next("visual");
        break;
    case 5: // forcefield
        if (menuOpen) mc->displayGuiScreen(nullptr);
        if (s_probePhaseT == 1)
        {
            setModuleState(Fly, false);
            probeSpawn(w, p, "Pig", 2, 0);
            probeSpawn(w, p, "Pig", 0, 2);
            probeSpawn(w, p, "Chicken", -2, 0);
        }
        if (s_probePhaseT == 20)
        {
            s_count0 = probeCountMobs(w, p, 4.0);
            setModuleOption(Forcefield, 2);
            setModuleState(Forcefield, true);
            PLOG("forcefield on, mobs within 4: %d\n", s_count0);
        }
        if (s_probePhaseT == 220)
        {
            PLOG("FORCEFIELD mobs within 4: before=%d after=%d flying=%d\n", s_count0, probeCountMobs(w, p, 4.0), p->capabilities.isFlying ? 1 : 0);
            setModuleState(Forcefield, false);
            next("forcefield");
        }
        break;
    case 6: // high jump + freecam
        if (menuOpen) mc->displayGuiScreen(nullptr);
        if (s_probePhaseT == 1)
        {
            setModuleState(HighJump, true);
            setModuleOption(HighJump, 2);
            p->jump();
            PLOG("HIGHJUMP motionY after jump()=%.3f (vanilla 0.42)\n", p->motionY);
            setModuleState(HighJump, false);
        }
        if (s_probePhaseT == 20)
        {
            s_px = p->posX; s_py = p->posY; s_pz = p->posZ;
            setModuleState(Freecam, true);
        }
        if (s_probePhaseT == 25)
            p->setPosition(p->posX + 3.0, p->posY - 6.0, p->posZ);
        if (s_probePhaseT == 60)
            PLOG("FREECAM noClip=%d inOpaque=%d y=%.2f (start %.2f)\n", p->noClip ? 1 : 0, p->isEntityInsideOpaqueBlock() ? 1 : 0, p->posY, s_py);
        if (s_probePhaseT == 70)
        {
            setModuleState(Freecam, false);
            PLOG("FREECAM off back=%.2f,%.2f,%.2f start=%.2f,%.2f,%.2f noClip=%d\n", p->posX, p->posY, p->posZ, s_px, s_py, s_pz, p->noClip ? 1 : 0);
            next("jump+freecam");
        }
        break;
    case 7: // nuker + build
        if (menuOpen) mc->displayGuiScreen(nullptr);
        if (s_probePhaseT == 1)
        {
            s_px = std::floor(p->posX); s_pz = std::floor(p->posZ); s_py = std::floor(p->boundingBox->minY);
            s_count0 = probeCountBlocks(w, (int)s_px, (int)s_py, (int)s_pz);
            setModuleOption(Nuker, 2);
            setModuleState(Nuker, true);
        }
        if (s_probePhaseT == 120)
        {
            PLOG("NUKER blocks around before=%d after=%d\n", s_count0, probeCountBlocks(w, (int)s_px, (int)s_py, (int)s_pz));
            setModuleState(Nuker, false);
            setModuleState(Build, true);
            setModuleOption(Build, 0);
            const int bx = (int)std::floor(p->posX) + 2, bz = (int)std::floor(p->posZ);
            const int by = w->getTopSolidOrLiquidBlock(bx, bz);
            w->setBlockAndMetadataWithNotify(bx, by, bz, 4, 0);
            AdminHooks::onBlockPlaced(bx, by - 1, bz, 1);
            int pole = 0;
            for (int i = 0; i < 6; ++i) if (w->getBlockId(bx, by + i, bz) == 4) ++pole;
            PLOG("BUILD pole cobble count=%d (want 6)\n", pole);
            setModuleState(Build, false);
            next("nuker+build");
        }
        break;
    case 8: // cave finder + wallhack + norender shot
        if (menuOpen) mc->displayGuiScreen(nullptr);
        if (s_probePhaseT == 1)
        {
            setModuleState(CaveFinder, true);
            setModuleState(WallHack, true);
            setModuleState(NoRender, true);
            PLOG("SHOT cavefinder xray=%d cave=%d\n", XRayMode::active() ? 1 : 0, XRayMode::caveMode() ? 1 : 0);
        }
        if (s_probePhaseT > 200)
        {
            setModuleState(CaveFinder, false);
            setModuleState(WallHack, false);
            setModuleState(NoRender, false);
            PLOG("cavefinder off xray=%d\n", XRayMode::active() ? 1 : 0);
            next("cavefinder");
        }
        break;
    case 9: // someone else's server
        if (menuOpen) mc->displayGuiScreen(nullptr);
        if (s_probePhaseT == 1)
            g_probeForceRemote = true;
        if (s_probePhaseT == 60)
            PLOG("REMOTE allowed=%d any=%d ESP=%d step=%.2f flying=%d\n", m_allowed ? 1 : 0, g_any ? 1 : 0, g_on[ESP] ? 1 : 0, p->stepHeight, p->capabilities.isFlying ? 1 : 0);
        if (s_probePhaseT == 120)
        {
            g_probeForceRemote = false;
            PLOG("SHOT remote message (chat)\n");
            next("remote");
        }
        break;
    case 10: // mod off
        if (s_probePhaseT == 20)
            ModManager::getInstance().setModEnabled("adminmode", false);
        if (s_probePhaseT == 40)
        {
            PLOG("MODOFF any=%d step=%.2f flying=%d\n", g_any ? 1 : 0, p->stepHeight, p->capabilities.isFlying ? 1 : 0);
            PLOG("END\n");
            next("end");
        }
        break;
    default:
        break;
    }
}
#endif
