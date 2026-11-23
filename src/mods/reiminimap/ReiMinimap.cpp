#include "ReiMinimap.h"
#include "GuiWaypointManager.h"
#include "GuiMinimapMenu.h"
#include "Minecraft.h"
#include "GuiIngame.h"
#include "HudSafeArea.h"
#include "FontRenderer.h"
#include "RenderEngine.h"
#include "EntityPlayerSP.h"
#include "EntityLiving.h"
#include "EntityMob.h"
#include "EntityAnimal.h"
#include "EntityWaterMob.h"
#include "EntitySlime.h"
#include "EntityGhast.h"
#include "EntityVillager.h"
#include "EntityGolem.h"
#include "World.h"
#include "WorldProvider.h"
#include "WorldInfo.h"
#include "BiomeGenBase.h"
#include "Chunk.h"
#include "Block.h"
#include "Material.h"
#include "MapColor.h"
#include "Tessellator.h"
#include "StringTranslate.h"
#include "platform/RenderAPI.h"
#include "platform/Storage.h"
#include "platform/Log.h"
#include "java/File.h"
#include "java/BufferedImage.h"
#include "SoundManager.h"
#include "ISaveHandler.h"

#include <cmath>
#include <cstring>
#include <algorithm>
#include <cstdio>
#include <cstdlib>

#if PLATFORM_PS2
#include "ps2/input/Ps2PadState.h"
#endif
#if PLATFORM_XBOX
#include "xbox/input/XboxInput.h"
#endif

static const int_t MAP_RES = 64;
static const int_t ROWS_PER_TICK = 1; // Ultra-light incremental refresh: 1 row * 64 cols (<0.04ms)
static const float_t PI_F = 3.14159265f;

static const int_t WAYPOINT_PALETTE[] = {
    0x00FF88, // Mint / Emerald (Default Spawn)
    0xFF2222, // Vibrant Red
    0x2288FF, // Bright Blue
    0xFFFF00, // Vibrant Yellow
    0xFF22FF, // Magenta / Pink
    0x00FFFF, // Cyan / Aqua
    0xFF8800, // Orange
    0xAA00FF, // Deep Purple
    0x77FF00, // Lime Green
    0xFF4488, // Coral Rose
    0xFFFFFF, // Pure White
    0x00A8FF  // Sky Blue
};
static const size_t NUM_PALETTE_COLORS = sizeof(WAYPOINT_PALETTE) / sizeof(WAYPOINT_PALETTE[0]);

static bool minimapIsSpanish()
{
    StringTranslate *tr = StringTranslate::getInstance();
    return (tr != nullptr && tr->getCurrentLanguage().rfind("es_", 0) == 0);
}

static void drawColoredRect(float_t x1, float_t y1, float_t x2, float_t y2, int_t color)
{
    if (x1 > x2) std::swap(x1, x2);
    if (y1 > y2) std::swap(y1, y2);
    float_t a = static_cast<float_t>((color >> 24) & 255) / 255.0f;
    float_t r = static_cast<float_t>((color >> 16) & 255) / 255.0f;
    float_t g = static_cast<float_t>((color >> 8) & 255) / 255.0f;
    float_t b = static_cast<float_t>(color & 255) / 255.0f;
    renderEnable(RenderCapability::Blend);
    renderDisable(RenderCapability::Texture2D);
    renderBlendFunc(RenderBlendFactor::SrcAlpha, RenderBlendFactor::OneMinusSrcAlpha);
    renderColor4f(r, g, b, a);
    Tessellator *tess = &Tessellator::instance;
    tess->startDrawingQuads();
    tess->setColorRGBA_F(r, g, b, a);
    tess->addVertex(x1, y2, 0.0);
    tess->addVertex(x2, y2, 0.0);
    tess->addVertex(x2, y1, 0.0);
    tess->addVertex(x1, y1, 0.0);
    tess->draw();
    renderEnable(RenderCapability::Texture2D);
}

static void drawColoredRing(float_t cx, float_t cy, float_t rInner, float_t rOuter, int_t color, int segments = 16)
{
    float_t a = static_cast<float_t>((color >> 24) & 255) / 255.0f;
    float_t r = static_cast<float_t>((color >> 16) & 255) / 255.0f;
    float_t g = static_cast<float_t>((color >> 8) & 255) / 255.0f;
    float_t b = static_cast<float_t>(color & 255) / 255.0f;
    renderEnable(RenderCapability::Blend);
    renderDisable(RenderCapability::Texture2D);
    renderBlendFunc(RenderBlendFactor::SrcAlpha, RenderBlendFactor::OneMinusSrcAlpha);
    renderColor4f(r, g, b, a);
    Tessellator *tess = &Tessellator::instance;
    tess->startDrawingQuads();
    tess->setColorRGBA_F(r, g, b, a);
    for (int i = 0; i < segments; ++i)
    {
        const float_t a0 = (2.0f * PI_F * static_cast<float_t>(i)) / static_cast<float_t>(segments);
        const float_t a1 = (2.0f * PI_F * static_cast<float_t>(i + 1)) / static_cast<float_t>(segments);
        const float_t c0 = std::cos(a0), s0 = std::sin(a0);
        const float_t c1 = std::cos(a1), s1 = std::sin(a1);
        tess->addVertex(cx + c0 * rInner, cy + s0 * rInner, 0.0);
        tess->addVertex(cx + c0 * rOuter, cy + s0 * rOuter, 0.0);
        tess->addVertex(cx + c1 * rOuter, cy + s1 * rOuter, 0.0);
        tess->addVertex(cx + c1 * rInner, cy + s1 * rInner, 0.0);
    }
    tess->draw();
    renderEnable(RenderCapability::Texture2D);
}

static void drawTexturedDisc(float_t cx, float_t cy, float_t radius, float_t uvScale, float_t rotRad, int segments = 24)
{
    const float_t cosR = std::cos(rotRad);
    const float_t sinR = std::sin(rotRad);
    Tessellator *tess = &Tessellator::instance;
    tess->startDrawingQuads();
    tess->setColorOpaque_I(0xFFFFFF);
    for (int i = 0; i < segments; ++i)
    {
        const float_t a0 = (2.0f * PI_F * static_cast<float_t>(i)) / static_cast<float_t>(segments);
        const float_t a1 = (2.0f * PI_F * static_cast<float_t>(i + 1)) / static_cast<float_t>(segments);
        const float_t x0 = std::cos(a0);
        const float_t y0 = std::sin(a0);
        const float_t x1 = std::cos(a1);
        const float_t y1 = std::sin(a1);
        // Rotate UV sample vector by rotRad
        const float_t ux0 = (x0 * cosR - y0 * sinR) * 0.5f * uvScale + 0.5f;
        const float_t uy0 = (x0 * sinR + y0 * cosR) * 0.5f * uvScale + 0.5f;
        const float_t ux1 = (x1 * cosR - y1 * sinR) * 0.5f * uvScale + 0.5f;
        const float_t uy1 = (x1 * sinR + y1 * cosR) * 0.5f * uvScale + 0.5f;
        // Degenerate quad as triangle slice (works natively with D3D8 quad path)
        tess->addVertexWithUV(cx, cy, 0.0, 0.5, 0.5);
        tess->addVertexWithUV(cx + x1 * radius, cy + y1 * radius, 0.0, ux1, uy1);
        tess->addVertexWithUV(cx + x0 * radius, cy + y0 * radius, 0.0, ux0, uy0);
        tess->addVertexWithUV(cx, cy, 0.0, 0.5, 0.5);
    }
    tess->draw();
}

ReiMinimap &ReiMinimap::getInstance()
{
    static ReiMinimap instance;
    return instance;
}

ReiMinimap::ReiMinimap()
    : m_mc(nullptr)
    , m_enabled(false)
    , m_updateTicks(0)
    , m_initialized(false)
    , m_deathCounter(0)
{
    for (int i = 0; i < 2; ++i)
    {
        m_mapTextureId[i] = -1;
        m_lastPlayerX[i] = -999999;
        m_lastPlayerZ[i] = -999999;
        m_lastPlayerY[i] = -999999;
        m_lastCaveActive[i] = false;
        m_caveState[i] = false;
        m_caveStableTicks[i] = 0;
        m_fastRows[i] = 0;
        m_scanRow[i] = 0;
        m_uploadCooldown[i] = 0;
        m_textureDirty[i] = false;
        m_waypointComboWasPressed[i] = false;
        m_waypointMenuComboWasPressed[i] = false;
        m_toastTimer[i] = 0;
        m_cachedBiomeX[i] = -999999;
        m_cachedBiomeZ[i] = -999999;
    }
}

ReiMinimap::~ReiMinimap()
{
    if (m_mc != nullptr && m_mc->renderEngine != nullptr)
    {
        for (int i = 0; i < 2; ++i)
        {
            if (m_mapTextureId[i] >= 0)
            {
                m_mc->renderEngine->deleteTexture(m_mapTextureId[i]);
                m_mapTextureId[i] = -1;
            }
        }
    }
}

size_t ReiMinimap::getMemoryBytes() const
{
    size_t cpuBytes = m_pixelData[0].capacity() + m_pixelData[1].capacity();
    if (m_mapImage)
        cpuBytes += static_cast<size_t>(MAP_RES * MAP_RES * 4);
    size_t gpuBytes = 0;
    for (int i = 0; i < 2; ++i)
    {
        if (m_mapTextureId[i] >= 0)
            gpuBytes += static_cast<size_t>(MAP_RES * MAP_RES * 4);
    }
    return cpuBytes + gpuBytes;
}

void ReiMinimap::ensureTexturesAllocated()
{
    if (!m_mapImage)
        m_mapImage = std::make_unique<BufferedImage>(MAP_RES, MAP_RES);
    for (int i = 0; i < 2; ++i)
    {
        if (m_pixelData[i].size() != static_cast<size_t>(MAP_RES * MAP_RES * 4))
            m_pixelData[i].assign(static_cast<size_t>(MAP_RES * MAP_RES * 4), 0);
        if (m_mapTextureId[i] < 0 && m_mc != nullptr && m_mc->renderEngine != nullptr)
            m_mapTextureId[i] = m_mc->renderEngine->allocateAndSetupTexture(m_mapImage.get());
    }
}

void ReiMinimap::setEnabled(bool en)
{
    m_enabled = en;
    if (en)
    {
        ensureTexturesAllocated();
        markMapDirty();
    }
    else
    {
        for (int i = 0; i < 2; ++i)
        {
            m_toastTimer[i] = 0;
            m_waypointComboWasPressed[i] = false;
            m_waypointMenuComboWasPressed[i] = false;
#if PLATFORM_XBOX
            (void)XboxInput::consumeWaypointRequest(i);
#endif
        }
    }
}

void ReiMinimap::init(Minecraft *mc)
{
    if (m_initialized)
        return;

    m_mc = mc;
    loadSettings();
    if (m_enabled)
        ensureTexturesAllocated();

    loadWaypoints();
    m_initialized = true;
    MC_LOG_INFO("mods", "Rei & Xaero Minimap initialized (enabled=%d, mem=%u KB)\n",
                m_enabled ? 1 : 0, static_cast<unsigned>(getMemoryBytes() / 1024));
}

void ReiMinimap::markMapDirty()
{
    for (int i = 0; i < 2; ++i)
    {
        m_lastPlayerX[i] = -999999;
        m_lastPlayerZ[i] = -999999;
        m_lastPlayerY[i] = -999999;
        m_scanRow[i] = 0;
        m_uploadCooldown[i] = 0;
        m_textureDirty[i] = true;
    }
}

float_t ReiMinimap::getZoomFactor() const
{
    static const float_t ZOOMS[] = { 0.5f, 1.0f, 2.0f, 4.0f };
    int idx = std::clamp<int>(m_settings.zoomIndex, 0, 3);
    return ZOOMS[idx];
}

std::string ReiMinimap::getZoomLabel() const
{
    static const char *const LABELS[] = { "0.5x", "1.0x", "2.0x", "4.0x" };
    int idx = std::clamp<int>(m_settings.zoomIndex, 0, 3);
    return LABELS[idx];
}

void ReiMinimap::cycleZoom(int_t playerIndex)
{
    m_settings.zoomIndex = (m_settings.zoomIndex + 1) % 4;
    saveSettings();
    markMapDirty();
    const bool isEs = minimapIsSpanish();
    showToast(playerIndex, (isEs ? "Zoom: " : "Zoom: ") + getZoomLabel(), 45);
    if (m_mc != nullptr && m_mc->sndManager != nullptr)
        m_mc->sndManager->playSoundFX("random.click", 1.0f, 1.1f);
}

void ReiMinimap::showToast(int_t playerIndex, const std::string &msg, int_t ticks)
{
    int idx = (playerIndex == 1) ? 1 : 0;
    m_toastMessage[idx] = msg;
    m_toastTimer[idx] = ticks;
}

void ReiMinimap::handlePlayerComboRequest(int_t playerIndex, EntityPlayer *player, int req, bool isSplit)
{
    if (player == nullptr || req <= 0 || m_mc == nullptr)
        return;

    const bool isEs = minimapIsSpanish();
    if (req == 1)
    {
        int_t px = static_cast<int_t>(std::floor(player->posX));
        int_t py = static_cast<int_t>(std::floor(player->posY));
        int_t pz = static_cast<int_t>(std::floor(player->posZ));
        char nameBuf[32];
        std::snprintf(nameBuf, sizeof(nameBuf),
            isSplit ? (playerIndex == 1 ? "P2 Waypoint %u" : "P1 Waypoint %u") : "Waypoint %u",
            static_cast<unsigned>(m_waypoints.size() + 1));
        addWaypoint(nameBuf, px, py, pz);
        showToast(playerIndex, isEs ? "Waypoint guardado!" : "Waypoint saved!", 60);
        if (m_mc->sndManager != nullptr)
            m_mc->sndManager->playSoundFX("random.orb", 1.0f, 1.0f);
    }
    else if (req == 2 && m_mc->currentScreen == nullptr)
    {
        if (isSplit && playerIndex == 1)
        {
            if (m_mc->player2Screen() == nullptr)
                m_mc->displayPlayerScreen(1, new GuiWaypointManager(1));
        }
        else
        {
            m_mc->displayGuiScreen(new GuiWaypointManager(playerIndex));
        }
    }
    else if (req == 3)
    {
        cycleZoom(playerIndex);
    }
    else if (req == 4 && m_mc->currentScreen == nullptr)
    {
        if (isSplit && playerIndex == 1)
        {
            if (m_mc->player2Screen() == nullptr)
                m_mc->displayPlayerScreen(1, new GuiMinimapMenu(nullptr, 1));
        }
        else
        {
            m_mc->displayGuiScreen(new GuiMinimapMenu(nullptr, playerIndex));
        }
    }
}

void ReiMinimap::update()
{
    if (!m_enabled || m_mc == nullptr || m_mc->theWorld == nullptr)
        return;

    ensureTexturesAllocated();

    // Detect world switch / entering a world
    std::string curDir;
    if (m_mc->theWorld->getSaveHandler() != nullptr)
        curDir = m_mc->theWorld->getSaveHandler()->getSaveDirectory();
    if (curDir != m_currentWorldDir)
    {
        m_currentWorldDir = curDir;
        m_deathCounter = 0;
        loadWaypoints();
        markMapDirty();
    }

    m_updateTicks++;
    for (int i = 0; i < 2; ++i)
    {
        if (m_toastTimer[i] > 0)
            m_toastTimer[i]--;
        if (m_uploadCooldown[i] > 0)
            m_uploadCooldown[i]--;
    }

    EntityPlayerSP *p1 = m_mc->thePlayerOne ? m_mc->thePlayerOne : m_mc->thePlayer;
    EntityPlayerSP *p2 = m_mc->thePlayer2;
    const bool isSplit = (m_mc->isSplitScreenActive() && p2 != nullptr);

#if PLATFORM_PS2
    if (p1 != nullptr)
    {
        const Ps2PadSnapshot &pad0 = ps2PadGetSnapshot(0);
        bool combo0 = (pad0.held & PS2_PAD_TRIANGLE) != 0 && (pad0.held & PS2_PAD_UP) != 0;
        if (combo0 && !m_waypointComboWasPressed[0])
            handlePlayerComboRequest(0, p1, 1, isSplit);
        m_waypointComboWasPressed[0] = combo0;

        bool menuCombo0 = (pad0.held & PS2_PAD_TRIANGLE) != 0 && (pad0.held & PS2_PAD_DOWN) != 0;
        if (menuCombo0 && !m_waypointMenuComboWasPressed[0] && m_mc->currentScreen == nullptr)
            handlePlayerComboRequest(0, p1, 2, isSplit);
        m_waypointMenuComboWasPressed[0] = menuCombo0;
    }
    if (isSplit && p2 != nullptr)
    {
        const Ps2PadSnapshot &pad1 = ps2PadGetSnapshot(1);
        bool combo1 = (pad1.held & PS2_PAD_TRIANGLE) != 0 && (pad1.held & PS2_PAD_UP) != 0;
        if (combo1 && !m_waypointComboWasPressed[1])
            handlePlayerComboRequest(1, p2, 1, isSplit);
        m_waypointComboWasPressed[1] = combo1;

        bool menuCombo1 = (pad1.held & PS2_PAD_TRIANGLE) != 0 && (pad1.held & PS2_PAD_DOWN) != 0;
        if (menuCombo1 && !m_waypointMenuComboWasPressed[1] && m_mc->currentScreen == nullptr)
            handlePlayerComboRequest(1, p2, 2, isSplit);
        m_waypointMenuComboWasPressed[1] = menuCombo1;
    }
#elif PLATFORM_XBOX
    if (p1 != nullptr)
    {
        const int req1 = XboxInput::consumeWaypointRequest(0);
        if (req1 > 0)
            handlePlayerComboRequest(0, p1, req1, isSplit);
    }
    if (isSplit && p2 != nullptr)
    {
        const int req2 = XboxInput::consumeWaypointRequest(1);
        if (req2 > 0)
            handlePlayerComboRequest(1, p2, req2, isSplit);
    }
#endif

    // Incremental map texture updates (never full 64x64 redraw per frame!)
    if (p1 != nullptr)
    {
        updatePlayerMapIncremental(0, p1, false);
        if ((m_updateTicks & 3) == 0 || m_cachedBiomeX[0] == -999999)
            refreshRadarAndBiomeCache(0, p1);
    }
    if (isSplit && p2 != nullptr)
    {
        updatePlayerMapIncremental(1, p2, false);
        if ((m_updateTicks & 3) == 2 || m_cachedBiomeX[1] == -999999)
            refreshRadarAndBiomeCache(1, p2);
    }
}

void ReiMinimap::refreshRadarAndBiomeCache(int_t playerIndex, EntityPlayer *player)
{
    if (m_mc == nullptr || m_mc->theWorld == nullptr || player == nullptr || playerIndex < 0 || playerIndex >= 2)
        return;

    const int_t px = static_cast<int_t>(std::floor(player->posX));
    const int_t pz = static_cast<int_t>(std::floor(player->posZ));
    if (m_settings.showBiome && (px != m_cachedBiomeX[playerIndex] || pz != m_cachedBiomeZ[playerIndex]))
    {
        m_cachedBiomeX[playerIndex] = px;
        m_cachedBiomeZ[playerIndex] = pz;
        BiomeGenBase *biome = m_mc->theWorld->getBiomeGenForCoords(px, pz);
        m_cachedBiome[playerIndex] = (biome != nullptr) ? biome->biomeName : "";
    }

    auto &dots = m_radarDots[playerIndex];
    dots.clear();
    if (!m_settings.entityRadar)
        return;

    const double ppx = player->posX;
    const double ppy = player->posY;
    const double ppz = player->posZ;
    const bool useCave = m_lastCaveActive[playerIndex];
    const double maxDy = useCave ? 12.0 : 28.0;
    const auto &entities = m_mc->theWorld->loadedEntityList;

    for (size_t i = 0; i < entities.size() && dots.size() < 32; ++i)
    {
        Entity *ent = entities[i];
        if (ent == nullptr || ent == player || ent->isDead)
            continue;
        if (std::abs(ent->posY - ppy) > maxDy)
            continue;
        const float_t rawDx = static_cast<float_t>(ent->posX - ppx);
        const float_t rawDz = static_cast<float_t>(ent->posZ - ppz);
        if (std::abs(rawDx) > 64.0f || std::abs(rawDz) > 64.0f)
            continue;

        int_t dotColor = 0;
        bool isOtherPlayer = false;
        if (dynamic_cast<EntityPlayer *>(ent) != nullptr)
        {
            dotColor = static_cast<int_t>(0xFF44FFFFu);
            isOtherPlayer = true;
        }
        else if (dynamic_cast<EntityMob *>(ent) != nullptr ||
                 dynamic_cast<EntitySlime *>(ent) != nullptr ||
                 dynamic_cast<EntityGhast *>(ent) != nullptr)
        {
            dotColor = static_cast<int_t>(0xFFFF3030u);
        }
        else if (dynamic_cast<EntityAnimal *>(ent) != nullptr ||
                 dynamic_cast<EntityWaterMob *>(ent) != nullptr ||
                 dynamic_cast<EntityVillager *>(ent) != nullptr ||
                 dynamic_cast<EntityGolem *>(ent) != nullptr)
        {
            dotColor = static_cast<int_t>(0xFF38EE38u);
        }
        else
        {
            continue;
        }
        dots.push_back({ rawDx, rawDz, dotColor, isOtherPlayer });
    }
}

bool ReiMinimap::isCaveActiveForPlayer(EntityPlayer *player) const
{
    if (m_settings.caveMode == MinimapCaveMode::Off)
        return false;
    if (m_settings.caveMode == MinimapCaveMode::Always)
        return true;
    if (player == nullptr || m_mc == nullptr || m_mc->theWorld == nullptr)
        return false;

    const int_t px = static_cast<int_t>(std::floor(player->posX));
    const int_t py = static_cast<int_t>(std::floor(player->posY));
    const int_t pz = static_cast<int_t>(std::floor(player->posZ));
    if (m_mc->theWorld->worldProvider != nullptr && m_mc->theWorld->worldProvider->isHellWorld)
        return true;
    // Auto cave mode: when player cannot see sky and has a solid roof overhead
    if (!m_mc->theWorld->canBlockSeeTheSky(px, py + 1, pz))
    {
        int_t height = m_mc->theWorld->getHeightValue(px, pz);
        if (height > py + 3)
            return true;
    }
    return false;
}

int_t ReiMinimap::getBlockColor(int_t blockId, int_t height, int_t northHeight) const
{
    if (blockId <= 0)
        return 0x00000000;

    int_t r = 120, g = 120, b = 120;

    switch (blockId)
    {
    case 8: case 9: // Water
        r = 46; g = 100; b = 254; break;
    case 10: case 11: // Lava
        r = 255; g = 85; b = 0; break;
    case 2: // Grass
        r = 85; g = 160; b = 48; break;
    case 3: case 60: // Dirt / Farmland
        r = 134; g = 96; b = 67; break;
    case 1: case 4: // Stone / Cobblestone
        r = 128; g = 128; b = 128; break;
    case 12: case 24: // Sand / Sandstone
        r = 216; g = 200; b = 136; break;
    case 13: // Gravel
        r = 136; g = 130; b = 130; break;
    case 17: case 5: // Wood / Planks
        r = 138; g = 108; b = 69; break;
    case 18: // Leaves
        r = 42; g = 112; b = 24; break;
    case 20: // Glass
        r = 192; g = 224; b = 240; break;
    case 78: case 80: // Snow
        r = 240; g = 240; b = 240; break;
    case 79: // Ice
        r = 160; g = 200; b = 240; break;
    case 81: // Cactus
        r = 24; g = 112; b = 24; break;
    case 82: // Clay
        r = 158; g = 159; b = 169; break;
    case 87: // Netherrack
        r = 112; g = 32; b = 32; break;
    case 88: // Soul sand
        r = 84; g = 64; b = 51; break;
    case 89: // Glowstone
        r = 228; g = 216; b = 140; break;
    case 49: // Obsidian
        r = 21; g = 18; b = 31; break;
    default:
        if (blockId < 256 && Block::blocksList[blockId] != nullptr)
        {
            Block *blk = Block::blocksList[blockId];
            if (blk->blockMaterial != nullptr && blk->blockMaterial->materialMapColor != nullptr)
            {
                int_t c = blk->blockMaterial->materialMapColor->colorValue;
                if (c != 0)
                {
                    r = (c >> 16) & 0xFF;
                    g = (c >> 8) & 0xFF;
                    b = c & 0xFF;
                }
            }
        }
        break;
    }

    // Hill / slope shading
    if (height > northHeight)
    {
        r = std::min<int_t>(255, r * 118 / 100);
        g = std::min<int_t>(255, g * 118 / 100);
        b = std::min<int_t>(255, b * 118 / 100);
    }
    else if (height < northHeight)
    {
        r = r * 82 / 100;
        g = g * 82 / 100;
        b = b * 82 / 100;
    }

    return (r & 0xFF) | ((g & 0xFF) << 8) | ((b & 0xFF) << 16) | static_cast<int_t>(0xFF000000u);
}

int_t ReiMinimap::getCaveColor(int_t blockId, int_t relY, int_t lightLevel) const
{
    if (blockId <= 0)
        return static_cast<int_t>(0xFF101016u); // Solid rock / no open cavern

    int_t r = 90, g = 115, b = 135;
    switch (blockId)
    {
    case 8: case 9:   r = 45;  g = 115; b = 255; break; // Water
    case 10: case 11: r = 255; g = 90;  b = 15;  break; // Lava
    case 14:          r = 240; g = 210; b = 60;  break; // Gold ore
    case 15:          r = 210; g = 175; b = 145; break; // Iron ore
    case 16:          r = 65;  g = 65;  b = 70;  break; // Coal ore
    case 56:          r = 80;  g = 230; b = 240; break; // Diamond ore
    case 73: case 74: r = 220; g = 45;  b = 45;  break; // Redstone ore
    case 48:          r = 75;  g = 135; b = 75;  break; // Mossy cobble (dungeon!)
    case 5: case 85:  r = 160; g = 120; b = 65;  break; // Mineshaft wood/fence
    case 66:          r = 185; g = 165; b = 120; break; // Rails
    default:
        // Depth-coded cave floor: cyan-green at player Y, warmer above, bluer below
        r = std::clamp<int_t>(85 + relY * 10, 35, 195);
        g = std::clamp<int_t>(145 - std::abs(relY) * 8, 55, 195);
        b = std::clamp<int_t>(165 - relY * 10, 45, 215);
        break;
    }

    const int_t shade = 55 + std::clamp<int_t>(lightLevel, 0, 15) * 3; // 55..100%
    r = (r * shade) / 100;
    g = (g * shade) / 100;
    b = (b * shade) / 100;
    return (r & 0xFF) | ((g & 0xFF) << 8) | ((b & 0xFF) << 16) | static_cast<int_t>(0xFF000000u);
}

bool ReiMinimap::scanMapRow(int_t playerIndex, EntityPlayer *player, int_t dy, int_t centerX, int_t centerZ, int_t stepBlocks, bool useCave)
{
    World *world = m_mc->theWorld;
    if (world == nullptr || dy < 0 || dy >= MAP_RES)
        return false;

    bool changed = false;

    const int_t playerY = static_cast<int_t>(std::floor(player->posY));
    const int_t wz = centerZ + (dy - MAP_RES / 2) * stepBlocks;
    const int_t chunkZ = wz >> 4;
    Chunk *cachedChunk = nullptr;
    int_t cachedChunkX = 0x7FFFFFFF;
    unsigned char *rowPtr = m_pixelData[playerIndex].data() + static_cast<size_t>(dy) * MAP_RES * 4;

    for (int_t dx = 0; dx < MAP_RES; ++dx)
    {
        const int_t wx = centerX + (dx - MAP_RES / 2) * stepBlocks;
        const int_t chunkX = wx >> 4;
        if (cachedChunk == nullptr || chunkX != cachedChunkX)
        {
            cachedChunk = world->getChunkFromBlockCoords(wx, wz);
            cachedChunkX = chunkX;
        }

        int_t col = static_cast<int_t>(0xFF181818u);
        if (cachedChunk != nullptr && !cachedChunk->isEmpty())
        {
            const int_t lx = wx & 15;
            const int_t lz = wz & 15;

            if (useCave)
            {
                // Find open cave air-above-floor near player's Y
                const int_t yTop = std::min<int_t>(124, playerY + 5);
                const int_t yBot = std::max<int_t>(2, playerY - 8);
                int_t foundFloor = 0;
                int_t foundY = playerY;
                int_t foundLight = 6;
                for (int_t cy = yTop; cy >= yBot; --cy)
                {
                    const int_t bid = cachedChunk->getBlockID(lx, cy, lz);
                    const bool isOpen = (bid == 0 || bid == 50 || bid == 75 || bid == 76 || bid == 66 || bid == 30);
                    if (isOpen)
                    {
                        const int_t below = cachedChunk->getBlockID(lx, cy - 1, lz);
                        if (below != 0 && below != 50 && below != 75 && below != 76)
                        {
                            foundFloor = below;
                            foundY = cy - 1;
                            foundLight = cachedChunk->getBlockLightValue(lx, cy, lz, 0);
                            break;
                        }
                    }
                }
                col = getCaveColor(foundFloor, foundY - playerY, foundLight);
            }
            else
            {
                int_t y = cachedChunk->getHeightValue(lx, lz) + 1;
                int_t blockId = 0;
                while (y > 1)
                {
                    blockId = cachedChunk->getBlockID(lx, y - 1, lz);
                    if (blockId != 0 && Block::blocksList[blockId] != nullptr)
                    {
                        Material *mat = Block::blocksList[blockId]->blockMaterial;
                        if (mat != nullptr && mat->materialMapColor != nullptr && mat->materialMapColor != MapColor::airColor)
                            break;
                    }
                    y--;
                }
                const int_t northHeight = (lz > 0) ? cachedChunk->getHeightValue(lx, lz - 1) : y;
                col = getBlockColor(blockId, y, northHeight);
            }
        }

        unsigned char *px = rowPtr + static_cast<size_t>(dx) * 4;
        const unsigned char nr = static_cast<unsigned char>(col & 0xFF);
        const unsigned char ng = static_cast<unsigned char>((col >> 8) & 0xFF);
        const unsigned char nb = static_cast<unsigned char>((col >> 16) & 0xFF);
        if (px[0] != nr || px[1] != ng || px[2] != nb || px[3] != 255)
        {
            px[0] = nr;
            px[1] = ng;
            px[2] = nb;
            px[3] = 255;
            changed = true;
        }
    }
    return changed;
}

void ReiMinimap::uploadPlayerTexture(int_t playerIndex)
{
    if (!m_mapImage || m_mapTextureId[playerIndex] < 0 || m_mc == nullptr || m_mc->renderEngine == nullptr)
        return;
    m_mapImage->setRGB(0, 0, MAP_RES, MAP_RES, m_pixelData[playerIndex].data());
    m_mc->renderEngine->setupTexture(m_mapImage.get(), m_mapTextureId[playerIndex]);
    m_textureDirty[playerIndex] = false;
    m_uploadCooldown[playerIndex] = 5; // At most ~4 uploads/sec, 0 when stationary & unchanged
}

void ReiMinimap::updatePlayerMapIncremental(int_t playerIndex, EntityPlayer *player, bool forceFull)
{
    if (m_mc == nullptr || player == nullptr || m_mc->theWorld == nullptr)
        return;
    if (playerIndex < 0 || playerIndex >= 2)
        return;
    ensureTexturesAllocated();
    if (m_mapTextureId[playerIndex] < 0)
        return;

    // 0.5x zoom scans every 2nd block (128x128 world area into 64x64 tex); 1x/2x/4x scan 1 block/texel and zoom UVs
    const int_t stepBlocks = (m_settings.zoomIndex == 0) ? 2 : 1;
    const int_t px = (static_cast<int_t>(std::floor(player->posX)) / stepBlocks) * stepBlocks;
    const int_t pz = (static_cast<int_t>(std::floor(player->posZ)) / stepBlocks) * stepBlocks;
    const int_t py = static_cast<int_t>(std::floor(player->posY));
    const bool firstInit = (m_lastPlayerX[playerIndex] == -999999);
    // Auto cave mode flips at every cave mouth / tree / overhang; only switch the map after the
    // new state held for ~1.5 s, otherwise the whole map was re-swept again and again.
    const bool rawCave = isCaveActiveForPlayer(player);
    if (firstInit || forceFull || m_settings.caveMode != MinimapCaveMode::Auto)
    {
        m_caveState[playerIndex] = rawCave;
        m_caveStableTicks[playerIndex] = 0;
    }
    else if (rawCave != m_caveState[playerIndex])
    {
        if (++m_caveStableTicks[playerIndex] >= 30)
        {
            m_caveState[playerIndex] = rawCave;
            m_caveStableTicks[playerIndex] = 0;
        }
    }
    else
        m_caveStableTicks[playerIndex] = 0;
    const bool useCave = m_caveState[playerIndex];

    const int_t dGridX = firstInit ? 999 : (px - m_lastPlayerX[playerIndex]) / stepBlocks;
    const int_t dGridZ = firstInit ? 999 : (pz - m_lastPlayerZ[playerIndex]) / stepBlocks;
    const bool caveChanged = (useCave != m_lastCaveActive[playerIndex]) ||
                             (useCave && std::abs(py - m_lastPlayerY[playerIndex]) >= 4);

    if (forceFull || firstInit || caveChanged || std::abs(dGridX) > 12 || std::abs(dGridZ) > 12)
    {
        m_lastPlayerX[playerIndex] = px;
        m_lastPlayerZ[playerIndex] = pz;
        m_lastPlayerY[playerIndex] = py;
        m_lastCaveActive[playerIndex] = useCave;
        if (forceFull || firstInit)
        {
            for (int_t dy = 0; dy < MAP_RES; ++dy)
                scanMapRow(playerIndex, player, dy, px, pz, stepBlocks, useCave);
            m_scanRow[playerIndex] = 0;
            m_fastRows[playerIndex] = 0;
            uploadPlayerTexture(playerIndex);
            return;
        }
        // Large move or cave height change: fast re-sweep (16 rows per tick, ~0.2 s) instead of
        // the idle sweep (1 row every 4 ticks = ~13 s of visible redrawing).
        m_scanRow[playerIndex] = 0;
        m_fastRows[playerIndex] = MAP_RES;
        m_textureDirty[playerIndex] = true;
    }
    else if (dGridX != 0 || dGridZ != 0)
    {
        // Fast in-memory shift of the 64x64 pixel buffer so already-scanned terrain is reused!
        unsigned char *buf = m_pixelData[playerIndex].data();
        const int_t rowBytes = MAP_RES * 4;
        if (dGridZ > 0)
        {
            for (int_t y = 0; y < MAP_RES - dGridZ; ++y)
                std::memmove(buf + y * rowBytes, buf + (y + dGridZ) * rowBytes, static_cast<size_t>(rowBytes));
        }
        else if (dGridZ < 0)
        {
            const int_t shift = -dGridZ;
            for (int_t y = MAP_RES - 1; y >= shift; --y)
                std::memmove(buf + y * rowBytes, buf + (y - shift) * rowBytes, static_cast<size_t>(rowBytes));
        }
        if (dGridX > 0)
        {
            const size_t keepBytes = static_cast<size_t>((MAP_RES - dGridX) * 4);
            for (int_t y = 0; y < MAP_RES; ++y)
                std::memmove(buf + y * rowBytes, buf + y * rowBytes + dGridX * 4, keepBytes);
        }
        else if (dGridX < 0)
        {
            const int_t shift = -dGridX;
            const size_t keepBytes = static_cast<size_t>((MAP_RES - shift) * 4);
            for (int_t y = 0; y < MAP_RES; ++y)
                std::memmove(buf + y * rowBytes + shift * 4, buf + y * rowBytes, keepBytes);
        }

        m_lastPlayerX[playerIndex] = px;
        m_lastPlayerZ[playerIndex] = pz;
        m_lastPlayerY[playerIndex] = py;

        // Scan newly exposed horizontal border rows right away (typically 1-2 rows)
        if (dGridZ > 0)
        {
            for (int_t y = MAP_RES - dGridZ; y < MAP_RES; ++y)
                scanMapRow(playerIndex, player, y, px, pz, stepBlocks, useCave);
        }
        else if (dGridZ < 0)
        {
            for (int_t y = 0; y < -dGridZ; ++y)
                scanMapRow(playerIndex, player, y, px, pz, stepBlocks, useCave);
        }
        // Scan newly exposed vertical border columns on the swept rows (or full edge if |dGridX| <= 2)
        if (dGridX != 0)
        {
            const int_t xStart = (dGridX > 0) ? (MAP_RES - dGridX) : 0;
            const int_t xEnd = (dGridX > 0) ? MAP_RES : (-dGridX);
            World *world = m_mc->theWorld;
            for (int_t dy = 0; dy < MAP_RES; ++dy)
            {
                const int_t wz = pz + (dy - MAP_RES / 2) * stepBlocks;
                for (int_t dx = xStart; dx < xEnd; ++dx)
                {
                    const int_t wx = px + (dx - MAP_RES / 2) * stepBlocks;
                    Chunk *ch = world->getChunkFromBlockCoords(wx, wz);
                    int_t col = static_cast<int_t>(0xFF181818u);
                    if (ch != nullptr && !ch->isEmpty())
                    {
                        const int_t lx = wx & 15;
                        const int_t lz = wz & 15;
                        if (useCave)
                        {
                            const int_t yTop = std::min<int_t>(124, py + 5);
                            const int_t yBot = std::max<int_t>(2, py - 8);
                            int_t foundFloor = 0, foundY = py, foundLight = 6;
                            for (int_t cy = yTop; cy >= yBot; --cy)
                            {
                                const int_t bid = ch->getBlockID(lx, cy, lz);
                                if (bid == 0 || bid == 50 || bid == 75 || bid == 76 || bid == 66)
                                {
                                    const int_t below = ch->getBlockID(lx, cy - 1, lz);
                                    if (below != 0 && below != 50 && below != 75 && below != 76)
                                    {
                                        foundFloor = below;
                                        foundY = cy - 1;
                                        foundLight = ch->getBlockLightValue(lx, cy, lz, 0);
                                        break;
                                    }
                                }
                            }
                            col = getCaveColor(foundFloor, foundY - py, foundLight);
                        }
                        else
                        {
                            int_t y = ch->getHeightValue(lx, lz) + 1;
                            int_t blockId = 0;
                            while (y > 1)
                            {
                                blockId = ch->getBlockID(lx, y - 1, lz);
                                if (blockId != 0 && Block::blocksList[blockId] != nullptr)
                                {
                                    Material *mat = Block::blocksList[blockId]->blockMaterial;
                                    if (mat != nullptr && mat->materialMapColor != nullptr && mat->materialMapColor != MapColor::airColor)
                                        break;
                                }
                                y--;
                            }
                            const int_t northH = (lz > 0) ? ch->getHeightValue(lx, lz - 1) : y;
                            col = getBlockColor(blockId, y, northH);
                        }
                    }
                    unsigned char *pxPtr = buf + (static_cast<size_t>(dy) * MAP_RES + static_cast<size_t>(dx)) * 4;
                    pxPtr[0] = static_cast<unsigned char>(col & 0xFF);
                    pxPtr[1] = static_cast<unsigned char>((col >> 8) & 0xFF);
                    pxPtr[2] = static_cast<unsigned char>((col >> 16) & 0xFF);
                    pxPtr[3] = 255;
                }
            }
        }
        m_textureDirty[playerIndex] = true;
    }

    // Incremental background sweep: refresh only ROWS_PER_TICK rows every 4 ticks,
    // and only mark texture dirty if at least one pixel actually changed (0 GPU uploads when idle!).
    const bool fastSweep = m_fastRows[playerIndex] > 0;
    if (fastSweep || (m_updateTicks & 3) == 0)
    {
        bool rowChanged = false;
        const int_t rows = fastSweep ? std::min<int_t>(16, m_fastRows[playerIndex]) : ROWS_PER_TICK;
        if (fastSweep)
            m_fastRows[playerIndex] -= rows;
        for (int_t r = 0; r < rows; ++r)
        {
            if (scanMapRow(playerIndex, player, m_scanRow[playerIndex], m_lastPlayerX[playerIndex], m_lastPlayerZ[playerIndex], stepBlocks, m_lastCaveActive[playerIndex]))
                rowChanged = true;
            m_scanRow[playerIndex] = (m_scanRow[playerIndex] + 1) % MAP_RES;
        }
        if (rowChanged)
            m_textureDirty[playerIndex] = true;
    }

    if (m_textureDirty[playerIndex] && m_uploadCooldown[playerIndex] <= 0)
    {
        uploadPlayerTexture(playerIndex);
    }
}

void ReiMinimap::render(GuiIngame *, int_t screenWidth, int_t screenHeight, float_t)
{
    if (!m_enabled || m_mc == nullptr || m_mc->thePlayer == nullptr || m_mc->theWorld == nullptr)
        return;

    const bool isSplit = (m_mc->isSplitScreenActive() && m_mc->thePlayer2 != nullptr);
    int_t playerIndex = 0;
    EntityPlayer *curPlayer = m_mc->thePlayer;
    if (isSplit && m_mc->thePlayer == m_mc->thePlayer2)
    {
        playerIndex = 1;
        curPlayer = m_mc->thePlayer2;
    }
    else if (m_mc->thePlayerOne != nullptr)
    {
        curPlayer = m_mc->thePlayerOne;
    }
    if (curPlayer == nullptr)
        curPlayer = m_mc->thePlayer;

    ensureTexturesAllocated();
    if (m_mapTextureId[playerIndex] < 0)
        return;

    if (m_lastPlayerX[playerIndex] == -999999)
        updatePlayerMapIncremental(playerIndex, curPlayer, true);

    renderDisable(RenderCapability::DepthTest);

    // HUD Safe Area + Adaptive sizing for 480p, 720p, and split-screen
    const HudSafeInsets safe = computeHudSafeInsets(m_mc, screenWidth, screenHeight);
    const bool isHighRes = (screenWidth > 360);
    const int_t mapSize = isSplit ? (isHighRes ? 56 : 44) : (isHighRes ? 76 : 58);
    const int_t margin = isSplit ? 4 : 6;
    const int_t posX = screenWidth - safe.right - mapSize - margin;
    const int_t posY = safe.top + margin;
    const float_t cx = posX + mapSize * 0.5f;
    const float_t cy = posY + mapSize * 0.5f;
    const float_t mapRadius = mapSize * 0.5f;

    const bool isRound = (m_settings.shape == MinimapShape::Round);
    const bool useCave = m_lastCaveActive[playerIndex];

    // Zoom UV scaling: 0.5x & 1.0x show full 64x64 texture (0.5x uses stepBlocks=2),
    // 2.0x crops center 32x32 (uvScale=0.5), 4.0x crops center 16x16 (uvScale=0.25)
    float_t uvScale = 1.0f;
    float_t visibleBlockRadius = 32.0f;
    switch (m_settings.zoomIndex)
    {
    case 0: uvScale = 1.0f;  visibleBlockRadius = 64.0f; break;
    case 1: uvScale = 1.0f;  visibleBlockRadius = 32.0f; break;
    case 2: uvScale = 0.5f;  visibleBlockRadius = 16.0f; break;
    case 3: uvScale = 0.25f; visibleBlockRadius = 8.0f;  break;
    default: uvScale = 1.0f; visibleBlockRadius = 32.0f; break;
    }

    const float_t rotRad = m_settings.rotateMap ? ((curPlayer->rotationYaw + 180.0f) * PI_F / 180.0f) : 0.0f;
    const float_t cosRot = std::cos(-rotRad);
    const float_t sinRot = std::sin(-rotRad);

    // 1. Frame & Map Texture
    const int_t frameAccent = useCave ? static_cast<int_t>(0xFF4488BBu) : static_cast<int_t>(0xFF777777u);
    if (isRound)
    {
        drawColoredRing(cx, cy, mapRadius - 0.5f, mapRadius + 2.5f, static_cast<int_t>(0xE0101010u), 24);
        drawColoredRing(cx, cy, mapRadius + 0.5f, mapRadius + 2.0f, frameAccent, 24);

        renderEnable(RenderCapability::Texture2D);
        renderEnable(RenderCapability::Blend);
        renderBlendFunc(RenderBlendFactor::SrcAlpha, RenderBlendFactor::OneMinusSrcAlpha);
        renderColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        renderBindTexture(m_mapTextureId[playerIndex]);
        drawTexturedDisc(cx, cy, mapRadius, uvScale, rotRad, 24);
    }
    else
    {
        drawColoredRect(posX - 1, posY - 1, posX + mapSize + 1, posY + mapSize + 1, static_cast<int_t>(0xC0000000u));
        drawColoredRect(posX - 2, posY - 2, posX + mapSize + 2, posY - 1, frameAccent);
        drawColoredRect(posX - 2, posY + mapSize + 1, posX + mapSize + 2, posY + mapSize + 2, frameAccent);
        drawColoredRect(posX - 2, posY - 2, posX - 1, posY + mapSize + 2, frameAccent);
        drawColoredRect(posX + mapSize + 1, posY - 2, posX + mapSize + 2, posY + mapSize + 2, frameAccent);

        renderEnable(RenderCapability::Texture2D);
        renderEnable(RenderCapability::Blend);
        renderBlendFunc(RenderBlendFactor::SrcAlpha, RenderBlendFactor::OneMinusSrcAlpha);
        renderColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        renderBindTexture(m_mapTextureId[playerIndex]);

        const float_t halfUv = 0.5f * uvScale;
        const float_t cR = std::cos(rotRad);
        const float_t sR = std::sin(rotRad);
        auto rotU = [&](float_t dx, float_t dy) { return 0.5f + (dx * cR - dy * sR) * halfUv; };
        auto rotV = [&](float_t dx, float_t dy) { return 0.5f + (dx * sR + dy * cR) * halfUv; };

        Tessellator *tess = &Tessellator::instance;
        tess->startDrawingQuads();
        tess->setColorOpaque_I(0xFFFFFF);
        tess->addVertexWithUV(posX,           posY + mapSize, 0.0, rotU(-1.0f,  1.0f), rotV(-1.0f,  1.0f));
        tess->addVertexWithUV(posX + mapSize, posY + mapSize, 0.0, rotU( 1.0f,  1.0f), rotV( 1.0f,  1.0f));
        tess->addVertexWithUV(posX + mapSize, posY,           0.0, rotU( 1.0f, -1.0f), rotV( 1.0f, -1.0f));
        tess->addVertexWithUV(posX,           posY,           0.0, rotU(-1.0f, -1.0f), rotV(-1.0f, -1.0f));
        tess->draw();
    }

    const float_t scale = mapRadius / visibleBlockRadius;
    const float_t innerLimit = mapRadius - 3.0f;

    // 2. Batched Entity Radar + Inside Waypoints (single D3D8 quad batch, 0 dynamic_casts in render!)
    const int_t px = static_cast<int_t>(std::floor(curPlayer->posX));
    const int_t py = static_cast<int_t>(std::floor(curPlayer->posY));
    const int_t pz = static_cast<int_t>(std::floor(curPlayer->posZ));
    const float_t borderR = mapRadius - 2.5f;
    Tessellator *tess = &Tessellator::instance;

    auto emitQuadColor = [&](float_t x1, float_t y1, float_t x2, float_t y2, int_t col) {
        tess->setColorRGBA((col >> 16) & 255, (col >> 8) & 255, col & 255, (col >> 24) & 255);
        tess->addVertex(x1, y2, 0.0);
        tess->addVertex(x2, y2, 0.0);
        tess->addVertex(x2, y1, 0.0);
        tess->addVertex(x1, y1, 0.0);
    };

    renderDisable(RenderCapability::Texture2D);
    renderEnable(RenderCapability::Blend);
    renderBlendFunc(RenderBlendFactor::SrcAlpha, RenderBlendFactor::OneMinusSrcAlpha);
    tess->startDrawingQuads();

    if (m_settings.entityRadar)
    {
        for (const auto &dot : m_radarDots[playerIndex])
        {
            if (std::abs(dot.dx) > visibleBlockRadius || std::abs(dot.dz) > visibleBlockRadius)
                continue;
            const float_t rx = dot.dx * cosRot - dot.dz * sinRot;
            const float_t rz = dot.dx * sinRot + dot.dz * cosRot;
            const float_t sx = cx + rx * scale;
            const float_t sy = cy + rz * scale;
            if (isRound)
            {
                const float_t ox = sx - cx, oy = sy - cy;
                if (ox * ox + oy * oy > innerLimit * innerLimit)
                    continue;
            }
            else if (std::abs(sx - cx) > innerLimit || std::abs(sy - cy) > innerLimit)
            {
                continue;
            }
            const float_t half = dot.isPlayer ? 2.0f : 1.3f;
            emitQuadColor(sx - half - 0.5f, sy - half - 0.5f, sx + half + 0.5f, sy + half + 0.5f, static_cast<int_t>(0xD0000000u));
            emitQuadColor(sx - half, sy - half, sx + half, sy + half, dot.color);
        }
    }

    // Inside waypoints batched in the same draw call; collect rim arrows
    struct RimArrow { float_t bx, by, ang; int_t col; };
    RimArrow rimArrows[16];
    int rimCount = 0;
    for (const auto &wp : m_waypoints)
    {
        if (!wp.enabled) continue;
        const float_t rawDx = static_cast<float_t>(wp.x - px);
        const float_t rawDz = static_cast<float_t>(wp.z - pz);
        const float_t rx = (rawDx * cosRot - rawDz * sinRot) * scale;
        const float_t rz = (rawDx * sinRot + rawDz * cosRot) * scale;

        const bool inside = isRound
            ? (rx * rx + rz * rz <= innerLimit * innerLimit)
            : (std::abs(rx) <= innerLimit && std::abs(rz) <= innerLimit);

        if (inside)
        {
            const float_t wxScreen = cx + rx;
            const float_t wzScreen = cy + rz;
            emitQuadColor(wxScreen - 2.5f, wzScreen - 2.5f, wxScreen + 2.5f, wzScreen + 2.5f, static_cast<int_t>(0xFF000000u));
            emitQuadColor(wxScreen - 1.5f, wzScreen - 1.5f, wxScreen + 1.5f, wzScreen + 1.5f, static_cast<int_t>(0xFF000000u) | wp.color);
        }
        else if (rimCount < 16)
        {
            float_t bx = cx, by = cy;
            if (isRound)
            {
                const float_t len = std::sqrt(rx * rx + rz * rz);
                if (len > 0.001f) { bx = cx + (rx / len) * borderR; by = cy + (rz / len) * borderR; }
            }
            else
            {
                const float_t maxAbs = std::max(std::abs(rx), std::abs(rz));
                if (maxAbs > 0.001f) { bx = cx + (rx / maxAbs) * borderR; by = cy + (rz / maxAbs) * borderR; }
            }
            rimArrows[rimCount++] = { bx, by, std::atan2(rz, rx) * (180.0f / PI_F), wp.color };
        }
    }

    // Batched rim waypoint arrows + player arrow (rotated in CPU -> zero push/pop matrix overhead!)
    auto emitArrow = [&](float_t ax, float_t ay, float_t deg, float_t s, int_t outlineCol, int_t fillCol) {
        const float_t rad = deg * (PI_F / 180.0f);
        const float_t ca = std::cos(rad);
        const float_t sa = std::sin(rad);
        auto pt = [&](float_t lx, float_t ly, float_t &ox, float_t &oy) {
            ox = ax + lx * ca - ly * sa;
            oy = ay + lx * sa + ly * ca;
        };
        float_t x0, y0, x1, y1, x2, y2;
        pt(-3.0f * s, 3.0f * s, x0, y0);
        pt( 3.0f * s, 3.0f * s, x1, y1);
        pt( 0.0f,    -4.6f * s, x2, y2);
        tess->setColorRGBA((outlineCol >> 16) & 255, (outlineCol >> 8) & 255, outlineCol & 255, 255);
        tess->addVertex(x0, y0, 0.0); tess->addVertex(x1, y1, 0.0); tess->addVertex(x2, y2, 0.0); tess->addVertex(x2, y2, 0.0);

        pt(-2.0f * s, 2.0f * s, x0, y0);
        pt( 2.0f * s, 2.0f * s, x1, y1);
        pt( 0.0f,    -3.5f * s, x2, y2);
        tess->setColorRGBA((fillCol >> 16) & 255, (fillCol >> 8) & 255, fillCol & 255, 255);
        tess->addVertex(x0, y0, 0.0); tess->addVertex(x1, y1, 0.0); tess->addVertex(x2, y2, 0.0); tess->addVertex(x2, y2, 0.0);
    };

    for (int i = 0; i < rimCount; ++i)
        emitArrow(rimArrows[i].bx, rimArrows[i].by, rimArrows[i].ang + 90.0f, 0.9f, 0x000000, rimArrows[i].col);

    // Player orientation arrow in center
    const float_t playerDeg = m_settings.rotateMap ? 0.0f : (curPlayer->rotationYaw + 180.0f);
    emitArrow(cx, cy, playerDeg, 1.05f, 0xFFFFFF, 0xFF2222);

    tess->draw();
    renderEnable(RenderCapability::Texture2D);

    // 3. Cardinal direction markers (N, S, W, E)
    FontRenderer *fr = m_mc->fontRenderer;
    if (fr != nullptr)
    {
        const float_t rimR = mapRadius - 4.0f;
        const struct { const char *lbl; float_t dx; float_t dz; int_t col; } dirs[4] = {
            { "N",  0.0f, -1.0f, 0xFF5555 },
            { "S",  0.0f,  1.0f, 0xCCCCCC },
            { "W", -1.0f,  0.0f, 0xCCCCCC },
            { "E",  1.0f,  0.0f, 0xCCCCCC }
        };
        for (int d = 0; d < 4; ++d)
        {
            float_t rx = dirs[d].dx * cosRot - dirs[d].dz * sinRot;
            float_t rz = dirs[d].dx * sinRot + dirs[d].dz * cosRot;
            if (!isRound)
            {
                const float_t m = std::max(std::abs(rx), std::abs(rz));
                if (m > 0.001f) { rx /= m; rz /= m; }
            }
            int_t tx = static_cast<int_t>(cx + rx * rimR) - 2;
            int_t ty = static_cast<int_t>(cy + rz * rimR) - 4;
            fr->drawStringWithShadow(dirs[d].lbl, tx, ty, dirs[d].col);
        }
    }

    // 6. Info lines below minimap: Coordinates, Biome, Cave/Zoom tag, Toast
    if (fr != nullptr)
    {
        int_t infoY = posY + mapSize + (isSplit ? 3 : 4);
        const int_t rightEdge = screenWidth - safe.right - 2;

        auto drawCenteredClamped = [&](const std::string &text, int_t y, int_t col, bool bg)
        {
            const int_t sw = fr->getStringWidth(text);
            int_t tx = posX + (mapSize - sw) / 2;
            if (tx + sw > rightEdge)
                tx = rightEdge - sw;
            if (tx < safe.left + 2)
                tx = safe.left + 2;
            if (bg)
                drawColoredRect(static_cast<float_t>(tx - 2), static_cast<float_t>(y - 1),
                                static_cast<float_t>(tx + sw + 2), static_cast<float_t>(y + 9),
                                static_cast<int_t>(0x90000000u));
            fr->drawStringWithShadow(text, tx, y, col);
        };

        if (m_settings.showCoords)
        {
            char coordBuf[48];
            std::snprintf(coordBuf, sizeof(coordBuf), "%d, %d, %d", px, py, pz);
            drawCenteredClamped(coordBuf, infoY, 0xFFFFFF, true);
            infoY += 10;
        }

        if (m_settings.showBiome && !isSplit)
        {
            if (!m_cachedBiome[playerIndex].empty())
            {
                std::string label = m_cachedBiome[playerIndex];
                if (useCave)
                    label += minimapIsSpanish() ? " [Cueva]" : " [Cave]";
                drawCenteredClamped(label, infoY, useCave ? 0x88DDFF : 0xCCCCCC, true);
                infoY += 10;
            }
        }
        else if (useCave)
        {
            drawCenteredClamped(minimapIsSpanish() ? "[Cueva]" : "[Cave]", infoY, 0x88DDFF, true);
            infoY += 10;
        }

        if (m_toastTimer[playerIndex] > 0 && !m_toastMessage[playerIndex].empty())
        {
            drawCenteredClamped(m_toastMessage[playerIndex], infoY + 1, 0x55FF55, true);
        }
    }

    renderEnable(RenderCapability::DepthTest);
}

int_t ReiMinimap::getNextWaypointColor() const
{
    return WAYPOINT_PALETTE[m_waypoints.size() % NUM_PALETTE_COLORS];
}

void ReiMinimap::addWaypoint(const std::string &name, int_t x, int_t y, int_t z, int_t color)
{
    if (color < 0)
        color = getNextWaypointColor();
    std::string cleanName = name;
    for (char &ch : cleanName)
    {
        if (ch == ':' || ch == '\n' || ch == '\r')
            ch = ' ';
    }
    if (cleanName.empty())
        cleanName = "Waypoint";
    m_waypoints.push_back({ cleanName, x, y, z, color, true });
    saveWaypoints();
}

void ReiMinimap::toggleWaypoint(size_t index)
{
    if (index < m_waypoints.size())
    {
        m_waypoints[index].enabled = !m_waypoints[index].enabled;
        saveWaypoints();
    }
}

void ReiMinimap::renameWaypoint(size_t index, const std::string &newName)
{
    if (index >= m_waypoints.size())
        return;
    std::string cleanName = newName;
    for (char &ch : cleanName)
    {
        if (ch == ':' || ch == '\n' || ch == '\r')
            ch = ' ';
    }
    if (!cleanName.empty())
    {
        m_waypoints[index].name = cleanName;
        saveWaypoints();
    }
}

void ReiMinimap::cycleWaypointColor(size_t index, int_t dir)
{
    if (index >= m_waypoints.size())
        return;
    size_t curIdx = 0;
    for (size_t i = 0; i < NUM_PALETTE_COLORS; ++i)
    {
        if (WAYPOINT_PALETTE[i] == m_waypoints[index].color)
        {
            curIdx = i;
            break;
        }
    }
    const int_t next = (static_cast<int_t>(curIdx) + dir + static_cast<int_t>(NUM_PALETTE_COLORS)) % static_cast<int_t>(NUM_PALETTE_COLORS);
    m_waypoints[index].color = WAYPOINT_PALETTE[next];
    saveWaypoints();
}

void ReiMinimap::removeWaypoint(size_t index)
{
    if (index < m_waypoints.size())
    {
        m_waypoints.erase(m_waypoints.begin() + index);
        saveWaypoints();
    }
}

void ReiMinimap::onPlayerDeath(EntityPlayer *player)
{
    if (!m_enabled || !m_settings.deathWaypoints || player == nullptr)
        return;
    const int_t px = static_cast<int_t>(std::floor(player->posX));
    const int_t py = static_cast<int_t>(std::floor(player->posY));
    const int_t pz = static_cast<int_t>(std::floor(player->posZ));
    m_deathCounter++;
    char nameBuf[48];
    const bool isEs = minimapIsSpanish();
    std::snprintf(nameBuf, sizeof(nameBuf), isEs ? "Muerte #%d" : "Death #%d", m_deathCounter);
    addWaypoint(nameBuf, px, py, pz, 0xFF2222);
    const int_t pIdx = (m_mc != nullptr && player == m_mc->thePlayer2) ? 1 : 0;
    showToast(pIdx, isEs ? "Punto de muerte guardado" : "Death point saved", 80);
}

std::string ReiMinimap::getWaypointsFilePath() const
{
    if (m_mc != nullptr && m_mc->theWorld != nullptr && m_mc->theWorld->getSaveHandler() != nullptr)
    {
        std::string saveDir = m_mc->theWorld->getSaveHandler()->getSaveDirectory();
        if (!saveDir.empty())
            return PlatformStorage::join(saveDir, "waypoints.txt");
    }
    File *dir = Minecraft::getMinecraftDir();
    if (dir != nullptr)
        return PlatformStorage::join(dir->toString(), "waypoints.txt");
    return "waypoints.txt";
}

std::string ReiMinimap::getSettingsFilePath() const
{
    File *dir = Minecraft::getMinecraftDir();
    if (dir != nullptr)
        return PlatformStorage::join(dir->toString(), "minimap.txt");
    return "minimap.txt";
}

void ReiMinimap::saveSettings()
{
    std::string content;
    content.reserve(256);
    content += "# Rei & Xaero Minimap Settings\n";
    content += "shape:" + std::to_string(static_cast<int>(m_settings.shape)) + "\n";
    content += "zoom:" + std::to_string(m_settings.zoomIndex) + "\n";
    content += "radar:" + std::string(m_settings.entityRadar ? "1" : "0") + "\n";
    content += "cave:" + std::to_string(static_cast<int>(m_settings.caveMode)) + "\n";
    content += "rotate:" + std::string(m_settings.rotateMap ? "1" : "0") + "\n";
    content += "coords:" + std::string(m_settings.showCoords ? "1" : "0") + "\n";
    content += "biome:" + std::string(m_settings.showBiome ? "1" : "0") + "\n";
    content += "death:" + std::string(m_settings.deathWaypoints ? "1" : "0") + "\n";
    PlatformStorage::writeFile(getSettingsFilePath(), content.data(), content.size());
}

void ReiMinimap::loadSettings()
{
    std::vector<unsigned char> bytes;
    if (!PlatformStorage::readFile(getSettingsFilePath(), bytes) || bytes.empty())
        return;

    std::string content(bytes.begin(), bytes.end());
    size_t start = 0;
    while (start < content.size())
    {
        size_t end = content.find('\n', start);
        std::string line = (end == std::string::npos) ? content.substr(start) : content.substr(start, end - start);
        start = (end == std::string::npos) ? content.size() : end + 1;
        while (!line.empty() && (line.back() == '\r' || line.back() == ' '))
            line.pop_back();
        if (line.empty() || line[0] == '#')
            continue;
        size_t sep = line.find(':');
        if (sep == std::string::npos)
            continue;
        std::string key = line.substr(0, sep);
        std::string val = line.substr(sep + 1);
        int iv = std::atoi(val.c_str());
        if (key == "shape") m_settings.shape = (iv == 1) ? MinimapShape::Round : MinimapShape::Square;
        else if (key == "zoom") m_settings.zoomIndex = std::clamp<int_t>(iv, 0, 3);
        else if (key == "radar") m_settings.entityRadar = (iv != 0);
        else if (key == "cave") m_settings.caveMode = static_cast<MinimapCaveMode>(std::clamp(iv, 0, 2));
        else if (key == "rotate") m_settings.rotateMap = (iv != 0);
        else if (key == "coords") m_settings.showCoords = (iv != 0);
        else if (key == "biome") m_settings.showBiome = (iv != 0);
        else if (key == "death") m_settings.deathWaypoints = (iv != 0);
    }
}

void ReiMinimap::loadWaypoints()
{
    m_waypoints.clear();
    std::string path = getWaypointsFilePath();
    std::vector<unsigned char> bytes;
    if (!PlatformStorage::readFile(path, bytes) || bytes.empty())
    {
        if (m_mc != nullptr && m_mc->theWorld != nullptr && m_mc->theWorld->getWorldInfo() != nullptr)
        {
            int_t sx = m_mc->theWorld->getWorldInfo()->getSpawnX();
            int_t sy = m_mc->theWorld->getWorldInfo()->getSpawnY();
            int_t sz = m_mc->theWorld->getWorldInfo()->getSpawnZ();
            m_waypoints.push_back({ "Spawn", sx, sy, sz, WAYPOINT_PALETTE[0], true });
        }
        else
        {
            m_waypoints.push_back({ "Spawn", 0, 64, 0, WAYPOINT_PALETTE[0], true });
        }
        return;
    }

    std::string content(bytes.begin(), bytes.end());
    size_t start = 0;
    while (start < content.size())
    {
        size_t end = content.find('\n', start);
        std::string line;
        if (end == std::string::npos)
        {
            line = content.substr(start);
            start = content.size();
        }
        else
        {
            line = content.substr(start, end - start);
            start = end + 1;
        }

        while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t'))
            line.pop_back();

        if (line.empty() || line[0] == '#')
            continue;

        size_t p1 = line.find(':');
        if (p1 == std::string::npos) continue;
        size_t p2 = line.find(':', p1 + 1);
        if (p2 == std::string::npos) continue;
        size_t p3 = line.find(':', p2 + 1);
        if (p3 == std::string::npos) continue;
        size_t p4 = line.find(':', p3 + 1);
        if (p4 == std::string::npos) continue;
        size_t p5 = line.find(':', p4 + 1);

        std::string name = line.substr(0, p1);
        int_t x = std::atoi(line.substr(p1 + 1, p2 - p1 - 1).c_str());
        int_t y = std::atoi(line.substr(p2 + 1, p3 - p2 - 1).c_str());
        int_t z = std::atoi(line.substr(p3 + 1, p4 - p3 - 1).c_str());
        int_t color = (p4 != std::string::npos && p5 != std::string::npos)
            ? std::atoi(line.substr(p4 + 1, p5 - p4 - 1).c_str())
            : WAYPOINT_PALETTE[m_waypoints.size() % NUM_PALETTE_COLORS];
        bool en = (p5 != std::string::npos) ? (line.substr(p5 + 1) == "1" || line.substr(p5 + 1) == "true") : true;

        m_waypoints.push_back({ name, x, y, z, color, en });
    }
}

void ReiMinimap::saveWaypoints()
{
    std::string path = getWaypointsFilePath();
    std::string content = "# Rei & Xaero Minimap Waypoints\n";
    for (const auto &wp : m_waypoints)
    {
        content += wp.name + ":" + std::to_string(wp.x) + ":" + std::to_string(wp.y) + ":" +
                   std::to_string(wp.z) + ":" + std::to_string(wp.color) + ":" + (wp.enabled ? "1" : "0") + "\n";
    }
    PlatformStorage::writeFile(path, content.data(), content.size());
}
