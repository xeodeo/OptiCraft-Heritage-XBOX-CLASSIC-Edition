#pragma once

#include "java/Type.h"
#include "Waypoint.h"
#include <vector>
#include <string>
#include <memory>

class Minecraft;
class GuiIngame;
class BufferedImage;
class EntityPlayer;

enum class MinimapShape : int
{
    Square = 0,
    Round = 1
};

enum class MinimapCaveMode : int
{
    Off = 0,
    Auto = 1,
    Always = 2
};

struct MinimapSettings
{
    MinimapShape shape = MinimapShape::Square;
    int_t zoomIndex = 1; // 0=0.5x, 1=1.0x, 2=2.0x, 3=4.0x
    bool entityRadar = true;
    MinimapCaveMode caveMode = MinimapCaveMode::Auto;
    bool rotateMap = false;
    bool showCoords = true;
    bool showBiome = true;
    bool deathWaypoints = true;
};

class ReiMinimap
{
public:
    static ReiMinimap &getInstance();

    void init(Minecraft *mc);
    void update();
    void render(GuiIngame *gui, int_t screenWidth, int_t screenHeight, float_t partialTick);

    bool isEnabled() const { return m_enabled; }
    void setEnabled(bool en);

    // Settings
    const MinimapSettings &getSettings() const { return m_settings; }
    MinimapSettings &getSettings() { return m_settings; }
    void saveSettings();
    void loadSettings();
    void cycleZoom(int_t playerIndex = 0);
    float_t getZoomFactor() const;
    std::string getZoomLabel() const;
    void markMapDirty();

    // Waypoints
    void addWaypoint(const std::string &name, int_t x, int_t y, int_t z, int_t color = -1);
    const std::vector<Waypoint> &getWaypoints() const { return m_waypoints; }
    size_t getWaypointCount() const { return m_waypoints.size(); }
    const Waypoint &getWaypoint(size_t index) const { return m_waypoints[index]; }
    void toggleWaypoint(size_t index);
    void renameWaypoint(size_t index, const std::string &newName);
    void cycleWaypointColor(size_t index, int_t dir = 1);
    void removeWaypoint(size_t index);

    void onPlayerDeath(EntityPlayer *player);
    void showToast(int_t playerIndex, const std::string &msg, int_t ticks = 60);

    void saveWaypoints();
    void loadWaypoints();

    // Memory telemetry (bytes used by CPU pixel buffers + GPU textures)
    size_t getMemoryBytes() const;

private:
    ReiMinimap();
    ~ReiMinimap();
    ReiMinimap(const ReiMinimap &) = delete;
    ReiMinimap &operator=(const ReiMinimap &) = delete;

    void ensureTexturesAllocated();
    void updatePlayerMapIncremental(int_t playerIndex, EntityPlayer *player, bool forceFull);
    bool scanMapRow(int_t playerIndex, EntityPlayer *player, int_t dy, int_t centerX, int_t centerZ, int_t stepBlocks, bool useCave);
    void refreshRadarAndBiomeCache(int_t playerIndex, EntityPlayer *player);
    void uploadPlayerTexture(int_t playerIndex);
    bool isCaveActiveForPlayer(EntityPlayer *player) const;
    int_t getBlockColor(int_t blockId, int_t height, int_t northHeight) const;
    int_t getCaveColor(int_t blockId, int_t relY, int_t lightLevel) const;
    std::string getWaypointsFilePath() const;
    std::string getSettingsFilePath() const;
    int_t getNextWaypointColor() const;
    void handlePlayerComboRequest(int_t playerIndex, EntityPlayer *player, int req, bool isSplit);

    Minecraft *m_mc;
    bool m_enabled;
    MinimapSettings m_settings;

    int_t m_mapTextureId[2];
    std::unique_ptr<BufferedImage> m_mapImage;
    std::vector<unsigned char> m_pixelData[2];

    int_t m_lastPlayerX[2];
    int_t m_lastPlayerZ[2];
    int_t m_lastPlayerY[2];
    bool m_lastCaveActive[2];
    int_t m_scanRow[2];
    int_t m_uploadCooldown[2];
    bool m_textureDirty[2];
    int_t m_updateTicks;

    std::vector<Waypoint> m_waypoints;
    bool m_initialized;
    bool m_waypointComboWasPressed[2];
    bool m_waypointMenuComboWasPressed[2];
    std::string m_toastMessage[2];
    int_t m_toastTimer[2];
    std::string m_currentWorldDir;
    int_t m_deathCounter;

    struct RadarDot
    {
        float_t dx;
        float_t dz;
        int_t color;
        bool isPlayer;
    };
    std::vector<RadarDot> m_radarDots[2];
    std::string m_cachedBiome[2];
    int_t m_cachedBiomeX[2];
    int_t m_cachedBiomeZ[2];
};
