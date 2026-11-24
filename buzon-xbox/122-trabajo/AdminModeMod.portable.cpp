#include "mods/adminmode/AdminModeMod.h"
#include "mods/adminmode/GuiAdminMenu.h"
#include "mods/ModManager.h"

#include "Minecraft.h"
#include "net/minecraft/src/AxisAlignedBB.h"
#include "net/minecraft/src/Block.h"
#include "net/minecraft/src/Container.h"
#include "net/minecraft/src/ContainerEnchantment.h"
#include "net/minecraft/src/EntityFish.h"
#include "net/minecraft/src/EntityLiving.h"
#include "net/minecraft/src/EntityPlayerSP.h"
#include "net/minecraft/src/EntityRenderer.h"
#include "net/minecraft/src/EnumMovingObjectType.h"
#include "net/minecraft/src/FoodStats.h"
#include "net/minecraft/src/FontRenderer.h"
#include "net/minecraft/src/GameSettings.h"
#include "net/minecraft/src/GuiEditSign.h"
#include "net/minecraft/src/GuiEnchantment.h"
#include "net/minecraft/src/GuiIngame.h"
#include "net/minecraft/src/IInventory.h"
#include "net/minecraft/src/InventoryPlayer.h"
#include "net/minecraft/src/ItemStack.h"
#include "net/minecraft/src/KeyBinding.h"
#include "net/minecraft/src/MovementInput.h"
#include "net/minecraft/src/MovingObjectPosition.h"
#include "net/minecraft/src/PlayerController.h"
#include "net/minecraft/src/SoundManager.h"
#include "net/minecraft/src/StringTranslate.h"
#include "net/minecraft/src/Tessellator.h"
#include "net/minecraft/src/TileEntity.h"
#include "net/minecraft/src/TileEntityChest.h"
#if __has_include("net/minecraft/src/TileEntityEnderChest.h")
#include "net/minecraft/src/TileEntityEnderChest.h"
#define ADMIN_HAS_ENDER_CHEST 1
#else
#define ADMIN_HAS_ENDER_CHEST 0
#endif
#include "net/minecraft/src/TileEntitySign.h"
#include "net/minecraft/src/Timer.h"
#include "net/minecraft/src/World.h"
#include "net/minecraft/src/WorldInfo.h"
#include "net/minecraft/src/XRayMode.h"
#include "java/File.h"
#include "platform/RenderAPI.h"
#include "platform/Storage.h"

#if __has_include("net/minecraft/src/ProtocolTranslator340.h")
#include "net/minecraft/src/ProtocolTranslator340.h"
#define ADMIN_HAS_OP_LEVEL 1
#else
#define ADMIN_HAS_OP_LEVEL 0
#endif

#if PLATFORM_XBOX
#include "xbox/input/XboxInput.h"
#endif
#if !PLATFORM_PS2 && !PLATFORM_WII && !PLATFORM_XBOX
#include "lwjgl/Keyboard.h"
#endif
#include "pc/lwjgl/Mouse.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace AdminHooks;

namespace AdminHooks
{
bool g_on[COUNT] = {};
bool g_any = false;
Entity *g_local[2] = {nullptr, nullptr};
Entity *g_collidingEntity = nullptr;

float highJumpFactor()
{
    AdminModeMod *m = AdminModeMod::getInstance();
    return m != nullptr ? m->highJumpFactor() : 1.0f;
}

bool fastBreakInstant()
{
    AdminModeMod *m = AdminModeMod::getInstance();
    return m != nullptr && m->fastBreakInstant();
}

bool noRenderItems()
{
    AdminModeMod *m = AdminModeMod::getInstance();
    return m != nullptr && m->noRenderItems();
}

void onBlockPlaced(int x, int y, int z, int side)
{
    if (!g_on[Build])
        return;
    if (AdminModeMod *m = AdminModeMod::getInstance())
        m->onBlockPlaced(x, y, z, side);
}

void renderWorldOverlay(float partialTicks)
{
    if (!(g_on[ESP] || g_on[Tracers] || g_on[ChestFinder] || g_on[BlockESP]))
        return;
    if (AdminModeMod *m = AdminModeMod::getInstance())
        m->renderWorldOverlay(partialTicks);
}
} // namespace AdminHooks

namespace
{
AdminModeMod *s_instance = nullptr;

bool adminIsSpanish()
{
    StringTranslate *tr = StringTranslate::getInstance();
    return tr != nullptr && tr->getCurrentLanguage().rfind("es_", 0) == 0;
}

struct ModuleDef
{
    const char *key;
    const char *nameEs;
    const char *nameEn;
    const char *descEs;
    const char *descEn;
    int optCount;
    int optDefault;
    const char *const *optEs;
    const char *const *optEn;
    bool localOnly;
};

const char *const kTargetEs[] = {"Jugadores", "Criaturas", "Todos"};
const char *const kTargetEn[] = {"Players", "Mobs", "All"};
const char *const kEggN[] = {"x2", "x3", "x5", "x8"};
const int kEggCount[] = {2, 3, 5, 8};
const char *const kEnchEs[] = {"Maximo", "5", "10", "15", "20", "25", "30"};
const char *const kEnchEn[] = {"Max", "5", "10", "15", "20", "25", "30"};
const int kEnchLevel[] = {0, 5, 10, 15, 20, 25, 30};
const char *const kSignEs[] = {"OptiCraft", "Coordenadas", "Nombre"};
const char *const kSignEn[] = {"OptiCraft", "Coordinates", "Name"};
const char *const kOreEs[] = {"Diamante", "Oro", "Hierro", "Carbon", "Redstone", "Lapislazuli", "Esmeralda", "Generador", "Lava"};
const char *const kOreEn[] = {"Diamond", "Gold", "Iron", "Coal", "Redstone", "Lapis", "Emerald", "Spawner", "Lava"};
const int kOreId[] = {56, 14, 15, 16, 73, 21, 129, 52, 11};
const char *const kBuildEs[] = {"Poste", "Pared", "Suelo"};
const char *const kBuildEn[] = {"Pole", "Wall", "Floor"};
const char *const kDerpEs[] = {"Aleatorio", "Girar"};
const char *const kDerpEn[] = {"Random", "Spin"};
const char *const kFastEs[] = {"Rapido", "Instantaneo"};
const char *const kFastEn[] = {"Fast", "Instant"};
const char *const kFloodN[] = {"2 s", "5 s", "10 s"};
const int kFloodTicks[] = {40, 100, 200};
const char *const kHealthEs[] = {"2 corazones", "3 corazones", "4 corazones", "6 corazones"};
const char *const kHealthEn[] = {"2 hearts", "3 hearts", "4 hearts", "6 hearts"};
const int kHealthHp[] = {4, 6, 8, 12};
const char *const kJumpN[] = {"x1.5", "x2", "x3", "x5"};
const float kJumpF[] = {1.5f, 2.0f, 3.0f, 5.0f};
const char *const kNoRenderEs[] = {"Particulas y clima", "+ objetos tirados"};
const char *const kNoRenderEn[] = {"Particles & weather", "+ dropped items"};
const char *const kNukerEs[] = {"Bloque elegido", "Rompibles al toque", "Todo", "Al picar (3x3x3)"};
const char *const kNukerEn[] = {"Chosen block", "Instant-break", "Everything", "On hit (3x3x3)"};
const char *const kStepN[] = {"1", "1.5", "2", "3"};
const float kStepH[] = {1.0f, 1.5f, 2.0f, 3.0f};
const char *const kTimerN[] = {"x0.5", "x1.5", "x2", "x3"};
const float kTimerF[] = {0.5f, 1.5f, 2.0f, 3.0f};

// Same order as AdminHooks::Module (Nodus "Enabled Mods" list, alphabetical).
const ModuleDef kModules[COUNT] = {
    {"aimbot", "Aimbot", "Aimbot", "Mira al objetivo mas cercano (8 bloques) y le pega", "Faces the nearest target (8 blocks) and hits it", 3, 1, kTargetEs, kTargetEn, false},
    {"autoegg", "AutoEgg", "AutoEgg", "Huevos y bolas de nieve: varios por pulsacion", "Eggs & snowballs: several per press", 4, 1, kEggN, kEggN, false},
    {"autoenchant", "AutoEnchant", "AutoEnchant", "En la mesa: encanta al nivel elegido (rebaraja)", "Enchanting table: picks the chosen level (rerolls)", 7, 0, kEnchEs, kEnchEn, false},
    {"autofish", "AutoFish", "AutoFish", "Recoge la cana al picar y la vuelve a lanzar", "Reels in on a bite and casts again", 0, 0, nullptr, nullptr, false},
    {"automine", "AutoMine", "AutoMine", "Pica solo el bloque al que miras", "Mines the block you look at by itself", 0, 0, nullptr, nullptr, false},
    {"autosign", "AutoSign", "AutoSign", "Escribe el cartel solo al ponerlo", "Fills a sign as soon as it is placed", 3, 0, kSignEs, kSignEn, false},
    {"autotool", "AutoTool", "AutoTool", "Coge la mejor herramienta/arma de la barra", "Picks the best hotbar tool/weapon", 0, 0, nullptr, nullptr, false},
    {"autowalk", "AutoWalk", "AutoWalk", "Anda hacia delante solo", "Walks forward by itself", 0, 0, nullptr, nullptr, false},
    {"blockesp", "BlockESP", "BlockESP", "Marca un mineral a 16 bloques a traves de las paredes", "Outlines an ore within 16 blocks through walls", 9, 0, kOreEs, kOreEn, false},
    {"bowaimbot", "BowAimbot", "BowAimbot", "Con el arco tensado apunta al objetivo (con caida)", "While drawing a bow, aims at the target (with drop)", 3, 1, kTargetEs, kTargetEn, false},
    {"build", "Build", "Build", "Al poner un bloque construye mas (solo mundo local)", "Placing a block builds more (local world only)", 3, 0, kBuildEs, kBuildEn, true},
    {"cavefinder", "CaveFinder", "CaveFinder", "Solo dibuja piedra: las cuevas se ven", "Draws only stone so caves stand out", 0, 0, nullptr, nullptr, false},
    {"chestfinder", "ChestFinder", "ChestFinder", "Marca los cofres cercanos a traves de las paredes", "Outlines nearby chests through walls", 0, 0, nullptr, nullptr, false},
    {"clickaimbot", "ClickAimbot", "ClickAimbot", "Al pegar al aire, gira y pega al mas cercano (6)", "Hitting the air turns and hits the nearest (6)", 0, 0, nullptr, nullptr, false},
    {"day", "Day", "Day", "Siempre de dia", "Always daytime", 0, 0, nullptr, nullptr, false},
    {"derp", "Derp", "Derp", "La cabeza gira sola (se ve en 3a persona)", "Head spins on its own (seen in 3rd person)", 2, 0, kDerpEs, kDerpEn, false},
    {"nametags", "Disable Nametags", "Disable Nametags", "Oculta los nombres sobre los jugadores", "Hides the names above players", 0, 0, nullptr, nullptr, false},
    {"esp", "ESP", "ESP", "Cajas alrededor de jugadores/criaturas", "Boxes around players/mobs", 3, 1, kTargetEs, kTargetEn, false},
    {"fastbreak", "FastBreak", "FastBreak", "Rompe bloques mas rapido y sin espera", "Breaks blocks faster with no delay", 2, 0, kFastEs, kFastEn, false},
    {"fastplace", "FastPlace", "FastPlace", "Pone bloques sin espera", "Places blocks with no delay", 0, 0, nullptr, nullptr, false},
    {"flood", "Flood", "Flood", "Repite un mensaje en el chat", "Repeats a chat message", 3, 1, kFloodN, kFloodN, false},
    {"fly", "Fly", "Fly", "Volar como en creativo (A dos veces)", "Creative-style flight (double A)", 0, 0, nullptr, nullptr, false},
    {"forcefield", "Forcefield", "Forcefield", "Pega a todo lo que este a 4 bloques", "Hits everything within 4 blocks", 3, 1, kTargetEs, kTargetEn, false},
    {"freecam", "Freecam", "Freecam", "Camara libre que atraviesa bloques; al apagar vuelves", "Free camera through blocks; switch off to return", 0, 0, nullptr, nullptr, false},
    {"fullbright", "FullBright", "FullBright", "Todo iluminado", "Everything lit", 0, 0, nullptr, nullptr, false},
    {"health", "Health", "Health", "Aviso con poca vida", "Low health warning", 4, 1, kHealthEs, kHealthEn, false},
    {"highjump", "HighJump", "HighJump", "Salta mas alto", "Jumps higher", 4, 1, kJumpN, kJumpN, false},
    {"nofall", "NoFall", "NoFall", "Sin dano por caida", "No fall damage", 0, 0, nullptr, nullptr, false},
    {"noflinch", "NoFlinch", "NoFlinch", "Los golpes no te empujan ni mueven la camara", "Hits do not push you or shake the camera", 0, 0, nullptr, nullptr, false},
    {"norender", "NoRender", "NoRender", "Sin particulas ni lluvia (mas fps)", "No particles or rain (more fps)", 2, 0, kNoRenderEs, kNoRenderEn, false},
    {"noswing", "NoSwing", "NoSwing", "Sin animacion de brazo", "No arm swing", 0, 0, nullptr, nullptr, false},
    {"notifications", "Notifications", "Notifications", "Avisa cuando un jugador entra o sale de vista", "Says when a player enters or leaves view", 0, 0, nullptr, nullptr, false},
    {"nuker", "Nuker", "Nuker", "Rompe bloques alrededor (solo mundo local)", "Breaks blocks around you (local world only)", 4, 0, kNukerEs, kNukerEn, true},
    {"noslowdown", "NoSlowDown", "NoSlowDown", "Comer/arco/telarana no frenan", "Eating, bows and webs do not slow you", 0, 0, nullptr, nullptr, false},
    {"sneak", "Sneak", "Sneak", "Siempre agachado (no caes por bordes)", "Always sneaking (no falling off edges)", 0, 0, nullptr, nullptr, false},
    {"spider", "Spider", "Spider", "Trepa paredes", "Climbs walls", 0, 0, nullptr, nullptr, false},
    {"sprint", "Sprint", "Sprint", "Corre siempre al andar hacia delante", "Always sprints when walking forward", 0, 0, nullptr, nullptr, false},
    {"step", "Step", "Step", "Sube bloques enteros sin saltar", "Steps up full blocks without jumping", 4, 0, kStepN, kStepN, false},
    {"timer", "Timer", "Timer", "Cambia la velocidad del juego", "Changes the game speed", 4, 2, kTimerN, kTimerN, false},
    {"tracers", "Tracers", "Tracers", "Lineas hacia jugadores/criaturas", "Lines to players/mobs", 3, 1, kTargetEs, kTargetEn, false},
    {"wallhack", "WallHack", "WallHack", "Criaturas visibles a traves de las paredes", "Mobs visible through walls", 0, 0, nullptr, nullptr, false},
    {"water", "Water", "Water", "Anda sobre el agua (agachate para hundirte)", "Walk on water (sneak to sink)", 0, 0, nullptr, nullptr, false},
    {"weather", "Weather", "Weather", "Quita la lluvia y las tormentas", "Clears rain and storms", 0, 0, nullptr, nullptr, false},
};

const char *const kColorAdmin = "\xc2\xa7" "6[Admin] ";

double interp(double last, double now, float pt)
{
    return last + (now - last) * (double)pt;
}

void addBoxLines(Tessellator *t, double x0, double y0, double z0, double x1, double y1, double z1)
{
    // 12 edges, GL_LINES.
    t->addVertex(x0, y0, z0); t->addVertex(x1, y0, z0);
    t->addVertex(x1, y0, z0); t->addVertex(x1, y0, z1);
    t->addVertex(x1, y0, z1); t->addVertex(x0, y0, z1);
    t->addVertex(x0, y0, z1); t->addVertex(x0, y0, z0);
    t->addVertex(x0, y1, z0); t->addVertex(x1, y1, z0);
    t->addVertex(x1, y1, z0); t->addVertex(x1, y1, z1);
    t->addVertex(x1, y1, z1); t->addVertex(x0, y1, z1);
    t->addVertex(x0, y1, z1); t->addVertex(x0, y1, z0);
    t->addVertex(x0, y0, z0); t->addVertex(x0, y1, z0);
    t->addVertex(x1, y0, z0); t->addVertex(x1, y1, z0);
    t->addVertex(x1, y0, z1); t->addVertex(x1, y1, z1);
    t->addVertex(x0, y0, z1); t->addVertex(x0, y1, z1);
}

bool attackKeyDown(Minecraft *mc)
{
    return mc->gameSettings != nullptr && mc->gameSettings->keyBindAttack != nullptr &&
           mc->gameSettings->keyBindAttack->pressed;
}

bool useKeyDown(Minecraft *mc)
{
    return mc->gameSettings != nullptr && mc->gameSettings->keyBindUseItem != nullptr &&
           mc->gameSettings->keyBindUseItem->pressed;
}
} // namespace

AdminModeMod::AdminModeMod()
    : m_mc(nullptr)
    , m_enabled(false)
    , m_allowed(false)
    , m_lastWorld(nullptr)
    , m_blockedWarned(false)
    , m_menuComboWasPressed(false)
    , m_tick(0)
    , m_freecamSaved(false)
    , m_fcX(0), m_fcY(0), m_fcZ(0)
    , m_fcYaw(0), m_fcPitch(0)
    , m_fcWasFlying(false), m_fcAllowFlying(false), m_fcNoDamage(false)
    , m_attackWasDown(false)
    , m_useWasDown(false)
    , m_autoMineForcing(false)
    , m_fishRecast(0)
    , m_nukerBlockId(0)
    , m_enchantCooldown(0)
    , m_floodTimer(0)
    , m_derpYaw(0.0f)
    , m_scanX(0), m_scanCX(0), m_scanCY(0), m_scanCZ(0), m_scanId(-1)
{
    s_instance = this;
    for (int i = 0; i < COUNT; ++i)
    {
        m_state[i] = false;
        m_option[i] = kModules[i].optDefault;
        m_prevOn[i] = false;
    }
    m_combatCooldown[0] = m_combatCooldown[1] = 0;
    m_healthWarned[0] = m_healthWarned[1] = false;
    m_enchantLast[0] = m_enchantLast[1] = m_enchantLast[2] = -1;
}

AdminModeMod *AdminModeMod::getInstance()
{
    return s_instance;
}

std::string AdminModeMod::getName() const
{
    return adminIsSpanish() ? "Modo admin (pruebas)" : "Admin mode (testing)";
}

std::string AdminModeMod::getDescription() const
{
    return adminIsSpanish()
        ? "Utilidades de Nodus 1.5.2 para probar: volar, sin caida, Freecam, ESP, Nuker... Solo en mundo propio, pantalla partida o servidor propio siendo op. Menu: Back+X."
        : "Nodus 1.5.2 testing tools: fly, no fall, Freecam, ESP, Nuker... Only in your own world, split screen or your own server as op. Menu: Back+X.";
}

std::string AdminModeMod::moduleName(int m)
{
    if (m < 0 || m >= COUNT)
        return "";
    return adminIsSpanish() ? kModules[m].nameEs : kModules[m].nameEn;
}

std::string AdminModeMod::moduleDesc(int m)
{
    if (m < 0 || m >= COUNT)
        return "";
    return adminIsSpanish() ? kModules[m].descEs : kModules[m].descEn;
}

int AdminModeMod::optionCount(int m)
{
    return (m >= 0 && m < COUNT) ? kModules[m].optCount : 0;
}

std::string AdminModeMod::optionLabel(int m, int opt)
{
    if (m < 0 || m >= COUNT || opt < 0 || opt >= kModules[m].optCount)
        return "";
    return adminIsSpanish() ? kModules[m].optEs[opt] : kModules[m].optEn[opt];
}

bool AdminModeMod::localWorldOnly(int m)
{
    return m >= 0 && m < COUNT && kModules[m].localOnly;
}

GuiScreen *AdminModeMod::createConfigScreen(GuiScreen *parent)
{
    return new GuiAdminMenu(parent, 0);
}

void AdminModeMod::setEnabled(bool enabled)
{
    m_enabled = enabled;
    if (!enabled)
        refreshFlags();
}

void AdminModeMod::onInit(Minecraft *mc)
{
    m_mc = mc;
    loadConfig();
}

bool AdminModeMod::inWorld() const
{
    return m_mc != nullptr && m_mc->theWorld != nullptr && m_mc->thePlayer != nullptr;
}

void AdminModeMod::setModuleState(int m, bool on)
{
    if (m < 0 || m >= COUNT)
        return;
    m_state[m] = on;
    if (m == Nuker)
        m_nukerBlockId = 0;
    refreshFlags();
}

void AdminModeMod::setModuleOption(int m, int opt)
{
    if (m < 0 || m >= COUNT || kModules[m].optCount <= 0)
        return;
    const int n = kModules[m].optCount;
    m_option[m] = ((opt % n) + n) % n;
    if (m == BlockESP)
    {
        m_blockHits.clear();
        m_scanId = -1;
    }
    if (m == Nuker)
        m_nukerBlockId = 0;
}

void AdminModeMod::allOff()
{
    for (int i = 0; i < COUNT; ++i)
        m_state[i] = false;
    refreshFlags();
}

float AdminModeMod::highJumpFactor() const
{
    const int o = std::clamp(m_option[HighJump], 0, 3);
    return kJumpF[o];
}

// --- persistence ------------------------------------------------------------

std::string AdminModeMod::configPath() const
{
    File *dir = Minecraft::getMinecraftDir();
    if (dir != nullptr)
        return PlatformStorage::join(dir->toString(), "adminmode.txt");
    return "adminmode.txt";
}

void AdminModeMod::saveConfig()
{
    std::string out = "# Admin mode (Nodus 1.5.2 modules): key=on,option\n";
    for (int i = 0; i < COUNT; ++i)
    {
        char line[64];
        std::snprintf(line, sizeof(line), "%s=%d,%d\n", kModules[i].key, m_state[i] ? 1 : 0, m_option[i]);
        out += line;
    }
    PlatformStorage::writeFile(configPath(), out.data(), out.size());
}

void AdminModeMod::loadConfig()
{
    std::vector<unsigned char> bytes;
    if (!PlatformStorage::readFile(configPath(), bytes) || bytes.empty())
        return;
    std::string content(bytes.begin(), bytes.end());
    size_t start = 0;
    while (start < content.size())
    {
        size_t end = content.find('\n', start);
        std::string line = (end == std::string::npos) ? content.substr(start) : content.substr(start, end - start);
        start = (end == std::string::npos) ? content.size() : end + 1;
        if (line.empty() || line[0] == '#')
            continue;
        const size_t eq = line.find('=');
        if (eq == std::string::npos)
            continue;
        const std::string key = line.substr(0, eq);
        int on = 0, opt = 0;
        if (std::sscanf(line.c_str() + eq + 1, "%d,%d", &on, &opt) < 1)
            continue;
        for (int i = 0; i < COUNT; ++i)
        {
            if (key == kModules[i].key)
            {
                m_state[i] = (on != 0);
                if (kModules[i].optCount > 0)
                    m_option[i] = std::clamp(opt, 0, kModules[i].optCount - 1);
                break;
            }
        }
    }
}

// --- flags & transitions ----------------------------------------------------

void AdminModeMod::refreshFlags()
{
    bool any = false;
    for (int i = 0; i < COUNT; ++i)
    {
        bool on = m_enabled && m_allowed && m_state[i] && inWorld();
        if (on && kModules[i].localOnly && m_mc->theWorld->multiplayerWorld)
            on = false;
        g_on[i] = on;
        any = any || on;
        if (on != m_prevOn[i])
        {
            m_prevOn[i] = on;
            onModuleChanged(i, on);
        }
    }
    g_any = any;
}

void AdminModeMod::onModuleChanged(int m, bool on)
{
    if (!on)
    {
        restoreModule(m);
        return;
    }
    EntityPlayerSP *p = nullptr;
    if (m_mc != nullptr)
        p = m_mc->inPlayer2Context() ? m_mc->thePlayerOne : m_mc->thePlayer;
    switch (m)
    {
    case Fly:
        for (Entity *e : g_local)
        {
            if (EntityPlayerSP *fp = dynamic_cast<EntityPlayerSP *>(e))
            {
                fp->capabilities.allowFlying = true;
                fp->capabilities.isFlying = true;
            }
        }
        break;
    case Freecam:
        if (p != nullptr)
        {
            m_freecamSaved = true;
            m_fcX = p->posX;
            m_fcY = p->posY;
            m_fcZ = p->posZ;
            m_fcYaw = p->rotationYaw;
            m_fcPitch = p->rotationPitch;
            m_fcWasFlying = p->capabilities.isFlying;
            m_fcAllowFlying = p->capabilities.allowFlying;
            m_fcNoDamage = p->capabilities.disableDamage;
            p->capabilities.allowFlying = true;
            p->capabilities.isFlying = true;
            p->capabilities.disableDamage = true;
            p->noClip = true;
        }
        break;
    case CaveFinder:
        XRayMode::setCaveMode(true);
        break;
    case Nuker:
        m_nukerBlockId = 0;
        break;
    case BlockESP:
        m_blockHits.clear();
        m_scanId = -1;
        break;
    case Notifications:
        m_seenPlayers.clear();
        break;
    default:
        break;
    }
}

void AdminModeMod::restoreModule(int m)
{
    Minecraft *mc = m_mc;
    EntityPlayerSP *players[2] = {nullptr, nullptr};
    if (mc != nullptr)
    {
        players[0] = mc->inPlayer2Context() ? mc->thePlayerOne : mc->thePlayer;
        players[1] = mc->thePlayer2;
    }
    switch (m)
    {
    case Fly:
        for (EntityPlayerSP *p : players)
        {
            if (p == nullptr || p->capabilities.isCreativeMode)
                continue;
            if (m == Fly && g_on[Freecam])
                continue;
            p->capabilities.allowFlying = false;
            p->capabilities.isFlying = false;
            p->fallDistance = 0.0f;
        }
        break;
    case Freecam:
        if (players[0] != nullptr && m_freecamSaved)
        {
            EntityPlayerSP *p = players[0];
            p->noClip = false;
            p->setPositionAndRotation(m_fcX, m_fcY, m_fcZ, m_fcYaw, m_fcPitch);
            p->motionX = p->motionY = p->motionZ = 0.0;
            p->fallDistance = 0.0f;
            p->capabilities.isFlying = m_fcWasFlying || (g_on[Fly] && !p->capabilities.isCreativeMode);
            p->capabilities.allowFlying = m_fcAllowFlying || g_on[Fly];
            p->capabilities.disableDamage = m_fcNoDamage;
        }
        m_freecamSaved = false;
        break;
    case Step:
        for (EntityPlayerSP *p : players)
            if (p != nullptr)
                p->stepHeight = 0.5f;
        break;
    case AdminHooks::Timer:
        if (mc != nullptr && mc->timer != nullptr)
            mc->timer->timerSpeed = 1.0f;
        break;
    case CaveFinder:
        XRayMode::setCaveMode(false);
        break;
    case AutoMine:
        if (m_autoMineForcing && mc != nullptr && mc->gameSettings != nullptr && mc->gameSettings->keyBindAttack != nullptr)
            mc->gameSettings->keyBindAttack->pressed = lwjgl::Mouse::isButtonDown(0);
        m_autoMineForcing = false;
        break;
    case BlockESP:
        m_blockHits.clear();
        m_blockScan.clear();
        m_scanId = -1;
        break;
    default:
        break;
    }
}

// --- menu combo -------------------------------------------------------------

void AdminModeMod::openMenu(int playerIndex)
{
    Minecraft *mc = m_mc;
    if (mc == nullptr || mc->theWorld == nullptr)
        return;
    if (playerIndex == 1 && mc->isSplitScreenActive() && mc->thePlayer2 != nullptr)
    {
        if (mc->player2Screen() == nullptr)
            mc->displayPlayerScreen(1, new GuiAdminMenu(nullptr, 1));
        return;
    }
    if (mc->currentScreen == nullptr)
        mc->displayGuiScreen(new GuiAdminMenu(nullptr, 0));
}

void AdminModeMod::handleMenuCombo()
{
#if PLATFORM_XBOX
    for (int i = 0; i < 2; ++i)
        if (XboxInput::consumeAdminMenuRequest(i))
            openMenu(i);
#elif !PLATFORM_PS2 && !PLATFORM_WII
    const bool down = lwjgl::Keyboard::isKeyDown(lwjgl::Keyboard::KEY_F9);
    if (down && !m_menuComboWasPressed && m_mc->currentScreen == nullptr)
        openMenu(0);
    m_menuComboWasPressed = down;
#endif
}

void AdminModeMod::chat(const std::string &es, const std::string &en)
{
    if (m_mc != nullptr && m_mc->ingameGUI != nullptr)
        m_mc->ingameGUI->addChatMessage(std::string(kColorAdmin) + (adminIsSpanish() ? es : en));
}

// --- tick ----------------------------------------------------------------------

void AdminModeMod::onTick()
{
    Minecraft *mc = m_mc;
    if (mc == nullptr)
        return;

    if (!m_enabled || mc->theWorld == nullptr || mc->thePlayer == nullptr)
    {
        m_allowed = false;
        g_local[0] = g_local[1] = nullptr;
        refreshFlags();
        m_lastWorld = mc->theWorld;
        return;
    }

    if (mc->theWorld != m_lastWorld)
    {
        m_lastWorld = mc->theWorld;
        m_blockedWarned = false;
        m_freecamSaved = false;
        m_seenPlayers.clear();
        m_blockHits.clear();
        m_scanId = -1;
        for (int i = 0; i < COUNT; ++i)
            m_prevOn[i] = false; // a new world: re-apply every module's start
    }

    // Local world or split screen: always. Server: only if it made us op (level >= 2).
    bool allowed = !mc->theWorld->multiplayerWorld;
#if ADMIN_HAS_OP_LEVEL
    if (!allowed && translator340::localOpLevel() >= 2)
        allowed = true;
#endif
    m_allowed = allowed;

    EntityPlayerSP *p1 = mc->inPlayer2Context() ? mc->thePlayerOne : mc->thePlayer;
    EntityPlayerSP *p2 = (mc->isSplitScreenActive() && mc->thePlayer2 != p1) ? mc->thePlayer2 : nullptr;
    g_local[0] = p1;
    g_local[1] = p2;

    handleMenuCombo();
    refreshFlags();

    if (!m_allowed)
    {
        bool wanted = false;
        for (int i = 0; i < COUNT; ++i)
            wanted = wanted || m_state[i];
        if (wanted && !m_blockedWarned)
        {
            m_blockedWarned = true;
            chat("\xc2\xa7" "cServidor ajeno: el modo admin no hace nada aqui.",
                 "\xc2\xa7" "cSomeone else's server: admin mode does nothing here.");
        }
        return;
    }

    if (!g_any)
        return;

    ++m_tick;
    if (p1 != nullptr)
    {
        tickPlayer(p1, true);
        tickPrimary(p1);
    }
    if (p2 != nullptr)
        tickPlayer(p2, false);

    if (g_on[AdminHooks::Timer] && mc->timer != nullptr)
        mc->timer->timerSpeed = kTimerF[std::clamp(m_option[AdminHooks::Timer], 0, 3)];

    World *w = mc->theWorld;
    if (g_on[Day] && (m_tick % 10) == 0)
    {
        const long_t t = w->getWorldTime();
        const long_t dayTime = t % 24000;
        if (dayTime < 0 || dayTime > 11500)
            w->setWorldTime(t - dayTime + 24000 + 1000);
    }
    if (g_on[Weather])
    {
        if (WorldInfo *info = w->getWorldInfo())
        {
            if (!w->multiplayerWorld && (info->isRaining() || info->isThundering()))
            {
                info->setRaining(false);
                info->setThundering(false);
                info->setRainTime(12000);
                info->setThunderTime(12000);
            }
        }
        w->setRainStrength(0.0f); // thunder strength is scaled by rain, so it goes too
    }
    if (g_on[FastPlace])
    {
        mc->rightClickDelayTimer = 0;
        mc->otherRightClickDelayTimer = 0;
    }
}

void AdminModeMod::tickPlayer(EntityPlayerSP *p, bool primary)
{
    const int idx = primary ? 0 : 1;

    if (g_on[NoFall] || g_on[Fly] || (primary && g_on[Freecam]))
        p->fallDistance = 0.0f;

    if (g_on[Fly])
        p->capabilities.allowFlying = true;

    if (primary && g_on[Freecam])
    {
        p->noClip = true;
        p->capabilities.allowFlying = true;
        p->capabilities.isFlying = true;
        p->capabilities.disableDamage = true;
    }

    if (g_on[Step])
        p->stepHeight = kStepH[std::clamp(m_option[Step], 0, 3)];

    if (g_on[Spider] && p->isCollidedHorizontally && !p->capabilities.isFlying)
        p->motionY = 0.2;

    if (g_on[Sprint] && p->movementInput != nullptr && p->movementInput->moveForward > 0.0f &&
        !p->isSneaking() && !p->isUsingItem() && !p->isCollidedHorizontally &&
        p->getFoodStats() != nullptr && p->getFoodStats()->getFoodLevel() > 6)
        p->setSprinting(true);

    if (g_on[Derp])
    {
        if (m_option[Derp] == 0)
            p->rotationYawHead = (float)(std::rand() % 360) - 180.0f;
        else
        {
            m_derpYaw += 40.0f;
            if (m_derpYaw > 360.0f)
                m_derpYaw -= 360.0f;
            p->rotationYawHead = p->rotationYaw + m_derpYaw;
        }
    }

    if (g_on[Health])
    {
        const int hp = p->getHealth();
        const int limit = kHealthHp[std::clamp(m_option[Health], 0, 3)];
        if (hp > 0 && hp <= limit && !m_healthWarned[idx])
        {
            m_healthWarned[idx] = true;
            char buf[96];
            std::snprintf(buf, sizeof(buf), "%s%d", primary ? "" : "J2 ", hp);
            chat(std::string("\xc2\xa7" "cPoca vida: ") + buf + "/20",
                 std::string("\xc2\xa7" "cLow health: ") + buf + "/20");
            if (m_mc->sndManager != nullptr)
                m_mc->sndManager->playSoundFX("random.orb", 1.0f, 0.5f);
        }
        else if (hp > limit)
        {
            m_healthWarned[idx] = false;
        }
    }

    tickCombat(p);
}

bool AdminModeMod::isTarget(EntityPlayerSP *p, Entity *e, int mode) const
{
    if (e == nullptr || e == p || isLocal(e) || e->isDead || !e->isLiving())
        return false;
    EntityLiving *l = static_cast<EntityLiving *>(e);
    if (l->getHealth() <= 0)
        return false;
    const bool isPlayer = e->isPlayer();
    if (mode == 0)
        return isPlayer;
    if (mode == 1)
        return !isPlayer;
    return true;
}

EntityLiving *AdminModeMod::nearestTarget(EntityPlayerSP *p, double range, int mode, bool needVisible) const
{
    World *w = m_mc->theWorld;
    EntityLiving *best = nullptr;
    double bestD = range * range;
    for (Entity *e : w->loadedEntityList)
    {
        if (!isTarget(p, e, mode))
            continue;
        const double d = p->getDistanceSqToEntity(e);
        if (d > bestD)
            continue;
        EntityLiving *l = static_cast<EntityLiving *>(e);
        if (needVisible && !p->canEntityBeSeen(l))
            continue;
        bestD = d;
        best = l;
    }
    return best;
}

void AdminModeMod::faceEntity(EntityPlayerSP *p, Entity *t, double extraY) const
{
    const double dx = t->posX - p->posX;
    const double dz = t->posZ - p->posZ;
    const double eyeY = p->boundingBox->minY + 1.62;
    const double ty = t->boundingBox->minY + (t->boundingBox->maxY - t->boundingBox->minY) * 0.75 + extraY;
    const double dy = ty - eyeY;
    const double horiz = std::sqrt(dx * dx + dz * dz);
    p->rotationYaw = (float)(std::atan2(dz, dx) * 180.0 / 3.141592653589793) - 90.0f;
    p->rotationPitch = (float)(-std::atan2(dy, horiz) * 180.0 / 3.141592653589793);
}

void AdminModeMod::attack(EntityPlayerSP *p, Entity *t)
{
    if (m_mc->playerController == nullptr)
        return;
    if (!g_on[NoSwing])
        p->swingItem();
    m_mc->playerController->attackEntity(p, t);
}

void AdminModeMod::tickCombat(EntityPlayerSP *p)
{
    const int idx = (p == g_local[1]) ? 1 : 0;
    if (m_combatCooldown[idx] > 0)
        --m_combatCooldown[idx];

    if (g_on[Forcefield] && m_combatCooldown[idx] == 0)
    {
        // Nodus: every 5 ticks, everything within 4 blocks that can be seen.
        const int mode = m_option[Forcefield];
        bool hit = false;
        for (Entity *e : m_mc->theWorld->loadedEntityList)
        {
            if (!isTarget(p, e, mode) || p->getDistanceSqToEntity(e) > 16.0)
                continue;
            if (!p->canEntityBeSeen(static_cast<EntityLiving *>(e)))
                continue;
            faceEntity(p, e, 0.0);
            attack(p, e);
            hit = true;
        }
        if (hit)
            m_combatCooldown[idx] = 5;
    }

    if (g_on[Aimbot])
    {
        if (EntityLiving *t = nearestTarget(p, 8.0, m_option[Aimbot], true))
        {
            faceEntity(p, t, 0.0);
            if (m_combatCooldown[idx] == 0 && p->getDistanceSqToEntity(t) < 4.5 * 4.5)
            {
                attack(p, t);
                m_combatCooldown[idx] = 7; // 350 ms like Nodus
            }
        }
    }

    if (g_on[BowAimbot] && p->isUsingItem())
    {
        ItemStack *s = p->getItemInUse();
        if (s != nullptr && s->itemID == 261)
        {
            if (EntityLiving *t = nearestTarget(p, 80.0, m_option[BowAimbot], true))
            {
                const double dist = std::sqrt(p->getDistanceSqToEntity(t));
                faceEntity(p, t, 0.12 * dist);
            }
        }
    }
}

void AdminModeMod::tickPrimary(EntityPlayerSP *p)
{
    Minecraft *mc = m_mc;
    World *w = mc->theWorld;
    const bool inGame = mc->currentScreen == nullptr && !mc->isPlayerScreenActive(0);
    const bool attackDown = attackKeyDown(mc);
    const bool useDown = useKeyDown(mc);
    const bool attackEdge = attackDown && !m_attackWasDown;
    const bool useEdge = useDown && !m_useWasDown;
    m_attackWasDown = attackDown;
    m_useWasDown = useDown;
    MovingObjectPosition *mop = mc->objectMouseOver;
    const bool lookTile = mop != nullptr && mop->typeOfHit == EnumMovingObjectType::TILE;
    const bool lookEntity = mop != nullptr && mop->typeOfHit == EnumMovingObjectType::ENTITY && mop->entityHit != nullptr;
    InventoryPlayer *inv = p->inventory;

    // AutoMine: hold the attack key while looking at a block.
    if (g_on[AutoMine] && mc->gameSettings->keyBindAttack != nullptr)
    {
        const bool want = inGame && lookTile;
        if (want)
        {
            mc->gameSettings->keyBindAttack->pressed = true;
            m_autoMineForcing = true;
        }
        else if (m_autoMineForcing)
        {
            mc->gameSettings->keyBindAttack->pressed = lwjgl::Mouse::isButtonDown(0);
            m_autoMineForcing = false;
        }
    }

    // AutoTool: best hotbar tool for the block, best weapon for the mob.
    if (g_on[AutoTool] && inGame && (attackDown || g_on[AutoMine]) && inv != nullptr)
    {
        int best = -1;
        float bestV = 1.0f;
        if (lookTile)
        {
            const int id = w->getBlockId(mop->blockX, mop->blockY, mop->blockZ);
            Block *b = (id > 0) ? Block::blocksList[id] : nullptr;
            if (b != nullptr)
            {
                for (int i = 0; i < 9; ++i)
                {
                    ItemStack *s = inv->mainInventory[i];
                    if (s == nullptr)
                        continue;
                    const float v = s->getStrVsBlock(b);
                    if (v > bestV)
                    {
                        bestV = v;
                        best = i;
                    }
                }
            }
        }
        else if (lookEntity)
        {
            for (int i = 0; i < 9; ++i)
            {
                ItemStack *s = inv->mainInventory[i];
                if (s == nullptr)
                    continue;
                const float v = (float)s->getDamageVsEntity(mop->entityHit);
                if (v > bestV)
                {
                    bestV = v;
                    best = i;
                }
            }
        }
        if (best >= 0)
            inv->currentItem = best;
    }

    // ClickAimbot: a hit at the air turns to the nearest target within 6 and hits it.
    if (g_on[ClickAimbot] && inGame && attackEdge && !lookEntity && !lookTile)
    {
        if (EntityLiving *t = nearestTarget(p, 6.0, 2, true))
        {
            faceEntity(p, t, 0.0);
            attack(p, t);
        }
    }

    // AutoEgg: extra throws for eggs (344) and snowballs (332).
    if (g_on[AutoEgg] && inGame && useEdge && inv != nullptr && mc->playerController != nullptr)
    {
        ItemStack *s = inv->getCurrentItem();
        if (s != nullptr && (s->itemID == 344 || s->itemID == 332))
        {
            const int extra = kEggCount[std::clamp(m_option[AutoEgg], 0, 3)] - 1;
            for (int i = 0; i < extra; ++i)
            {
                s = inv->getCurrentItem();
                if (s == nullptr || s->stackSize <= 0)
                    break;
                mc->playerController->sendUseItem(p, w, s);
                if (s->stackSize == 0)
                {
                    delete s;
                    inv->mainInventory[inv->currentItem] = nullptr;
                    break;
                }
            }
        }
    }

    // AutoFish: reel in on a bite, cast again a moment later.
    if (g_on[AutoFish] && inv != nullptr && mc->playerController != nullptr)
    {
        ItemStack *s = inv->getCurrentItem();
        const bool rod = s != nullptr && s->itemID == 346;
        if (rod && p->fishEntity != nullptr && p->fishEntity->getTicksCatchable() > 0)
        {
            mc->playerController->sendUseItem(p, w, s);
            m_fishRecast = 15;
        }
        else if (m_fishRecast > 0 && --m_fishRecast == 0 && rod && p->fishEntity == nullptr && inGame)
        {
            mc->playerController->sendUseItem(p, w, s);
        }
    }

    // AutoSign: fill the sign editor as soon as it opens and close it.
    if (g_on[AutoSign])
    {
        if (GuiEditSign *gs = dynamic_cast<GuiEditSign *>(mc->currentScreen))
        {
            if (TileEntitySign *sign = gs->getSign())
            {
                std::string lines[4];
                switch (m_option[AutoSign])
                {
                case 1:
                {
                    char buf[48];
                    lines[0] = "X Y Z";
                    std::snprintf(buf, sizeof(buf), "%d %d %d", sign->xCoord, sign->yCoord, sign->zCoord);
                    lines[1] = buf;
                    break;
                }
                case 2:
                    lines[1] = p->username;
                    break;
                default:
                    lines[1] = "OptiCraft";
                    lines[2] = "Heritage";
                    break;
                }
                for (int i = 0; i < 4; ++i)
                    sign->signText[i] = lines[i].substr(0, 15);
                sign->onInventoryChanged();
                mc->displayGuiScreen(nullptr);
            }
        }
    }

    if (g_on[AutoEnchant])
        tickAutoEnchant(p);

    if (g_on[Nuker])
        tickNuker(p, attackEdge && inGame && lookTile);

    if (g_on[BlockESP] && (m_tick & 1) == 0)
        tickBlockEsp(p);

    if (g_on[Notifications] && (m_tick % 10) == 0)
        tickNotifications(p);

    if (g_on[Flood])
    {
        if (++m_floodTimer >= kFloodTicks[std::clamp(m_option[Flood], 0, 2)])
        {
            m_floodTimer = 0;
            p->sendChatMessage("OptiCraft Heritage Edition");
        }
    }
}

void AdminModeMod::tickAutoEnchant(EntityPlayerSP *p)
{
    Minecraft *mc = m_mc;
    ContainerEnchantment *ce = dynamic_cast<ContainerEnchantment *>(p->craftingInventory);
    if (ce == nullptr || dynamic_cast<GuiEnchantment *>(mc->currentScreen) == nullptr || mc->playerController == nullptr)
    {
        m_enchantLast[0] = m_enchantLast[1] = m_enchantLast[2] = -1;
        return;
    }
    if (m_enchantCooldown > 0)
    {
        --m_enchantCooldown;
        return;
    }
    if (ce->tableInventory == nullptr || ce->tableInventory->getStackInSlot(0) == nullptr)
        return;
    if (ce->enchantLevels[0] == m_enchantLast[0] && ce->enchantLevels[1] == m_enchantLast[1] &&
        ce->enchantLevels[2] == m_enchantLast[2])
        return;
    m_enchantLast[0] = ce->enchantLevels[0];
    m_enchantLast[1] = ce->enchantLevels[1];
    m_enchantLast[2] = ce->enchantLevels[2];

    int want = kEnchLevel[std::clamp(m_option[AutoEnchant], 0, 6)];
    if (want == 0)
        want = std::min(p->experienceLevel, 30);
    for (int i = 0; i < 3; ++i)
    {
        if (ce->enchantLevels[i] > 0 && ce->enchantLevels[i] == want &&
            (p->experienceLevel >= want || p->capabilities.isCreativeMode))
        {
            if (ce->enchantItem(p, i))
                mc->playerController->sendEnchantPacket(ce->windowId, i);
            m_enchantCooldown = 10;
            return;
        }
    }
    // None matches: take the item out and put it back for new levels (Nodus does the same).
    mc->playerController->windowClick(ce->windowId, 0, 0, false, p);
    mc->playerController->windowClick(ce->windowId, 0, 0, false, p);
    m_enchantCooldown = 2;
}

void AdminModeMod::tickNuker(EntityPlayerSP *p, bool hitEdge)
{
    Minecraft *mc = m_mc;
    World *w = mc->theWorld;
    PlayerController *pc = mc->playerController;
    if (pc == nullptr || w->multiplayerWorld)
        return;
    const int mode = m_option[Nuker];
    MovingObjectPosition *mop = mc->objectMouseOver;

    auto breakable = [&](int id) -> bool {
        if (id <= 0 || id == 7 || (id >= 8 && id <= 11))
            return false;
        Block *b = Block::blocksList[id];
        return b != nullptr && b->blockHardness >= 0.0f;
    };

    if (mode == 3)
    {
        if (!hitEdge || mop == nullptr)
            return;
        for (int dx = -1; dx <= 1; ++dx)
            for (int dy = -1; dy <= 1; ++dy)
                for (int dz = -1; dz <= 1; ++dz)
                {
                    const int x = mop->blockX + dx, y = mop->blockY + dy, z = mop->blockZ + dz;
                    if (y > 0 && breakable(w->getBlockId(x, y, z)))
                        pc->sendBlockRemoved(x, y, z, 1);
                }
        return;
    }

    if (mode == 0 && m_nukerBlockId == 0)
    {
        // The first block hit chooses what to break.
        if (hitEdge && mop != nullptr)
        {
            m_nukerBlockId = w->getBlockId(mop->blockX, mop->blockY, mop->blockZ);
            if (m_nukerBlockId > 0 && Block::blocksList[m_nukerBlockId] != nullptr)
                chat("Nuker: bloque elegido", "Nuker: block chosen");
        }
        return;
    }

    const int px = (int)std::floor(p->posX);
    const int pz = (int)std::floor(p->posZ);
    const int feetY = (int)std::floor(p->boundingBox->minY);
    const double eyeY = p->boundingBox->minY + 1.62;
    int broken = 0;
    // A few blocks per tick: every break relights and rebuilds a chunk section.
    for (int dy = 4; dy >= 0 && broken < 4; --dy)
        for (int dx = -4; dx <= 4 && broken < 4; ++dx)
            for (int dz = -4; dz <= 4 && broken < 4; ++dz)
            {
                const int x = px + dx, y = feetY + dy, z = pz + dz;
                if (y <= 0 || y > 255)
                    continue;
                const int id = w->getBlockId(x, y, z);
                if (!breakable(id))
                    continue;
                if (mode == 0 && id != m_nukerBlockId)
                    continue;
                if (mode == 1 && Block::blocksList[id]->blockHardness != 0.0f)
                    continue;
                const double ddx = x + 0.5 - p->posX, ddy = y + 0.5 - eyeY, ddz = z + 0.5 - p->posZ;
                if (ddx * ddx + ddy * ddy + ddz * ddz > 25.0)
                    continue;
                pc->sendBlockRemoved(x, y, z, 1);
                ++broken;
            }
}

void AdminModeMod::tickBlockEsp(EntityPlayerSP *p)
{
    World *w = m_mc->theWorld;
    const int target = kOreId[std::clamp(m_option[BlockESP], 0, 8)];
    const int R = 16;
    if (m_scanId != target || m_scanX > R)
    {
        if (m_scanId == target && m_scanX > R)
            m_blockHits.swap(m_blockScan);
        m_blockScan.clear();
        m_scanId = target;
        m_scanX = -R;
        m_scanCX = (int)std::floor(p->posX);
        m_scanCY = (int)std::floor(p->posY);
        m_scanCZ = (int)std::floor(p->posZ);
    }
    // Two x slices (33 x 33 blocks each) every other tick: a full sweep in ~1.7 s.
    for (int n = 0; n < 2 && m_scanX <= R; ++n, ++m_scanX)
    {
        const int x = m_scanCX + m_scanX;
        const int y0 = std::max(1, m_scanCY - R), y1 = std::min(255, m_scanCY + R);
        for (int y = y0; y <= y1; ++y)
            for (int z = m_scanCZ - R; z <= m_scanCZ + R; ++z)
            {
                const int id = w->getBlockId(x, y, z);
                const bool match = id == target || (target == 73 && id == 74) || (target == 11 && id == 10);
                if (match && m_blockScan.size() < 400)
                    m_blockScan.push_back({x, y, z});
            }
    }
}

void AdminModeMod::tickNotifications(EntityPlayerSP *p)
{
    World *w = m_mc->theWorld;
    std::vector<std::string> now;
    for (EntityPlayer *ep : w->playerEntities)
    {
        if (ep == nullptr || isLocal(ep) || ep == p)
            continue;
        now.push_back(ep->username);
    }
    for (const std::string &n : now)
        if (std::find(m_seenPlayers.begin(), m_seenPlayers.end(), n) == m_seenPlayers.end())
            chat(n + " ha entrado en tu rango de vision.", n + " has entered visual range.");
    for (const std::string &n : m_seenPlayers)
        if (std::find(now.begin(), now.end(), n) == now.end())
            chat(n + " ha salido de tu rango de vision.", n + " has left visual range.");
    m_seenPlayers.swap(now);
}

// --- hooks ------------------------------------------------------------------

void AdminModeMod::onBlockPlaced(int x, int y, int z, int side)
{
    Minecraft *mc = m_mc;
    if (mc == nullptr || mc->theWorld == nullptr || mc->theWorld->multiplayerWorld)
        return;
    World *w = mc->theWorld;
    static const int offs[6][3] = {{0, -1, 0}, {0, 1, 0}, {0, 0, -1}, {0, 0, 1}, {-1, 0, 0}, {1, 0, 0}};
    int px = x, py = y, pz = z;
    if (side >= 0 && side < 6)
    {
        px += offs[side][0];
        py += offs[side][1];
        pz += offs[side][2];
    }
    int id = w->getBlockId(px, py, pz);
    if (id == 0)
    {
        px = x; py = y; pz = z; // replaced the clicked block (snow, grass)
        id = w->getBlockId(px, py, pz);
    }
    if (id <= 0)
        return;
    const int meta = w->getBlockMetadata(px, py, pz);
    auto put = [&](int bx, int by, int bz) {
        if (by > 0 && by < 256 && w->getBlockId(bx, by, bz) == 0)
            w->setBlockAndMetadataWithNotify(bx, by, bz, id, meta);
    };
    EntityPlayerSP *p = mc->thePlayer;
    switch (m_option[Build])
    {
    case 0: // pole: 6 tall
        for (int i = 1; i < 6; ++i)
            put(px, py + i, pz);
        break;
    case 1: // wall 6x6, across the way the player looks
    {
        const bool alongX = p != nullptr && std::fabs(std::sin(p->rotationYaw * 3.14159265f / 180.0f)) < 0.7071f;
        for (int a = 0; a < 6; ++a)
            for (int b = 0; b < 6; ++b)
                put(px + (alongX ? b : 0), py + a, pz + (alongX ? 0 : b));
        break;
    }
    default: // floor 6x6 centred
        for (int a = -3; a < 3; ++a)
            for (int b = -3; b < 3; ++b)
                put(px + a, py, pz + b);
        break;
    }
}

void AdminModeMod::renderWorldOverlay(float pt)
{
    Minecraft *mc = m_mc;
    if (mc == nullptr || mc->theWorld == nullptr || mc->renderViewEntity == nullptr)
        return;
    World *w = mc->theWorld;
    EntityLiving *view = mc->renderViewEntity;
    const double vx = interp(view->lastTickPosX, view->posX, pt);
    const double vy = interp(view->lastTickPosY, view->posY, pt);
    const double vz = interp(view->lastTickPosZ, view->posZ, pt);
    Tessellator *t = &Tessellator::instance;

    if (mc->entityRenderer != nullptr)
        mc->entityRenderer->disableLightmap(pt);
    renderDisable(RenderCapability::Texture2D);
    renderDisable(RenderCapability::Lighting);
    renderDisable(RenderCapability::Fog);
    renderDisable(RenderCapability::DepthTest);
    renderDepthMask(false);
    renderEnable(RenderCapability::Blend);
    renderBlendFunc(RenderBlendFactor::SrcAlpha, RenderBlendFactor::OneMinusSrcAlpha);
    renderLineWidth(2.0f);

    const int espMode = m_option[ESP];
    const int trMode = m_option[Tracers];
    if (g_on[ESP] || g_on[Tracers])
    {
        // Players blue, mobs red.
        for (int pass = 0; pass < 2; ++pass)
        {
            const bool players = (pass == 0);
            if (players)
                renderColor4f(0.3f, 0.6f, 1.0f, 0.9f);
            else
                renderColor4f(1.0f, 0.25f, 0.2f, 0.9f);
            int verts = 0;
            t->startDrawing(renderPrimitiveValue(RenderPrimitive::Lines));
            for (Entity *e : w->loadedEntityList)
            {
                if (e == nullptr || e == view || isLocal(e) || e->isDead || !e->isLiving() || e->isPlayer() != players)
                    continue;
                const bool esp = g_on[ESP] && isTarget(nullptr, e, espMode);
                const bool tr = g_on[Tracers] && isTarget(nullptr, e, trMode);
                if (!esp && !tr)
                    continue;
                const double ex = interp(e->lastTickPosX, e->posX, pt) - e->posX;
                const double ey = interp(e->lastTickPosY, e->posY, pt) - e->posY;
                const double ez = interp(e->lastTickPosZ, e->posZ, pt) - e->posZ;
                const AxisAlignedBB *bb = e->boundingBox;
                const double x0 = bb->minX + ex - vx, y0 = bb->minY + ey - vy, z0 = bb->minZ + ez - vz;
                const double x1 = bb->maxX + ex - vx, y1 = bb->maxY + ey - vy, z1 = bb->maxZ + ez - vz;
                if (x0 * x0 + z0 * z0 > 96.0 * 96.0)
                    continue;
                if (esp)
                {
                    addBoxLines(t, x0, y0, z0, x1, y1, z1);
                    verts += 24;
                }
                if (tr)
                {
                    t->addVertex(0.0, 0.0, 0.0);
                    t->addVertex((x0 + x1) * 0.5, (y0 + y1) * 0.5, (z0 + z1) * 0.5);
                    verts += 2;
                }
                if (verts > 1500)
                {
                    t->draw();
                    t->startDrawing(renderPrimitiveValue(RenderPrimitive::Lines));
                    verts = 0;
                }
            }
            t->draw();
        }
    }

    if (g_on[ChestFinder])
    {
        renderColor4f(1.0f, 0.75f, 0.1f, 0.9f);
        t->startDrawing(renderPrimitiveValue(RenderPrimitive::Lines));
        int verts = 0;
        for (TileEntity *te : w->loadedTileEntityList)
        {
            bool chest = te != nullptr && dynamic_cast<TileEntityChest *>(te) != nullptr;
#if ADMIN_HAS_ENDER_CHEST
            chest = chest || (te != nullptr && dynamic_cast<TileEntityEnderChest *>(te) != nullptr);
#endif
            if (!chest)
                continue;
            const double x0 = te->xCoord - vx, y0 = te->yCoord - vy, z0 = te->zCoord - vz;
            if (x0 * x0 + z0 * z0 > 64.0 * 64.0)
                continue;
            addBoxLines(t, x0 + 0.0625, y0, z0 + 0.0625, x0 + 0.9375, y0 + 0.875, z0 + 0.9375);
            verts += 24;
            if (verts > 1500)
            {
                t->draw();
                t->startDrawing(renderPrimitiveValue(RenderPrimitive::Lines));
                verts = 0;
            }
        }
        t->draw();
    }

    if (g_on[BlockESP] && !m_blockHits.empty())
    {
        renderColor4f(0.2f, 1.0f, 1.0f, 0.9f);
        t->startDrawing(renderPrimitiveValue(RenderPrimitive::Lines));
        int verts = 0;
        for (const BlockPos &b : m_blockHits)
        {
            const double x0 = b.x - vx, y0 = b.y - vy, z0 = b.z - vz;
            addBoxLines(t, x0, y0, z0, x0 + 1.0, y0 + 1.0, z0 + 1.0);
            verts += 24;
            if (verts > 1500)
            {
                t->draw();
                t->startDrawing(renderPrimitiveValue(RenderPrimitive::Lines));
                verts = 0;
            }
        }
        t->draw();
    }

    renderColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    renderDisable(RenderCapability::Blend);
    renderDepthMask(true);
    renderEnable(RenderCapability::DepthTest);
    renderEnable(RenderCapability::Fog);
    renderEnable(RenderCapability::Texture2D);
    if (mc->entityRenderer != nullptr)
        mc->entityRenderer->enableLightmap(pt);
}

// --- HUD ----------------------------------------------------------------------

void AdminModeMod::onRenderGameOverlay(GuiIngame *gui, int_t screenWidth, int_t screenHeight, float_t partialTick)
{
    (void)gui;
    (void)screenWidth;
    (void)screenHeight;
    (void)partialTick;
    Minecraft *mc = m_mc;
    if (mc == nullptr || mc->fontRenderer == nullptr || mc->theWorld == nullptr || Minecraft::isDebugInfoEnabled())
        return;
    if (!m_allowed)
        return;
    if (!g_any)
        return;
    // Nodus "Enabled Mods" list, top left.
    int y = 2;
    int shown = 0;
    for (int i = 0; i < COUNT && shown < 12; ++i)
    {
        if (!g_on[i])
            continue;
        mc->fontRenderer->drawStringWithShadow(moduleName(i), 2, y, 0x55FF55);
        y += 9;
        ++shown;
    }
}

