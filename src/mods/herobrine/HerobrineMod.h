#pragma once

#include "mods/IMod.h"
#include "java/Type.h"
#include <string>
#include <vector>

class World;
class EntityPlayer;
class EntityHerobrine;
class GuiScreen;

// Faithful clean-room C++ implementation of tkelly's original iLoveYou Herobrine plugin
// behavior (exact altar + AltarEvent, GhostEvent, HeroDigger, DiamondEvent, ThiefEvent).
// Anything destructive or taking items from the player defaults to OFF and is toggleable
// with clear warnings in GuiHerobrineMenu.
struct HerobrineConfig
{
    bool altarRitual = true;    // Exact tkelly altar sequence: nightfall, torch break, Herobrine summon + chat
    bool ghostEvent = true;     // iLoveYou GhostEvent: spawns ~17 blocks away with redstone torch
    bool altarCurse = true;     // ON like the original iLoveYou: fire curse & deleteAltar() (user 22-11)
    bool diggerEvent = false;   // OFF by default: iLoveYou HeroDigger (digs tunnel + underground room)
    bool diamondEvent = false;  // OFF by default: iLoveYou DiamondEvent (digs & restores shaft to diamond ore)
    bool thiefEvent = false;    // OFF by default: iLoveYou ThiefEvent (drops/steals items from distant chests)
    bool legendExtras = true;   // Optional extras: altar lightning, eerie sounds, leafless trees, signs, pyramids
};

class HerobrineMod : public IMod
{
public:
    HerobrineMod();
    ~HerobrineMod() override = default;

    static HerobrineMod *getInstance();

    std::string getId() const override { return "herobrine"; }
    std::string getName() const override { return "Herobrine Classic"; }
    std::string getVersion() const override { return "v1.1"; }
    std::string getDescription() const override { return "Original tkelly (iLoveYou) altar summoning, Ghost, Digger, Diamond & Thief events."; }
    std::string getAuthor() const override { return "OptiCraft"; }

    bool isEnabled() const override { return m_enabled; }
    void setEnabled(bool state) override;

    bool hasConfigScreen() const override { return true; }
    GuiScreen *createConfigScreen(GuiScreen *parent) override;

    void onInit(Minecraft *mc) override;
    void onTick() override;

    // Called from BlockFire::onBlockAdded when fire is placed in a world
    static bool onFirePlaced(World *world, int_t x, int_t y, int_t z);

    // State & config queries & persistence
    bool isSummoned() const { return m_summoned; }
    int_t getSightingCount() const { return m_sightings; }
    int_t getMythEventCount() const { return m_mythEvents; }
    HerobrineConfig &getConfig() { return m_config; }
    const HerobrineConfig &getConfig() const { return m_config; }
    void saveState();
    void loadState();
    void saveConfig();
    void loadConfig();

    // Helpers
    static bool checkAltarStructure(World *world, int_t fireX, int_t fireY, int_t fireZ);
    EntityHerobrine *spawnGhostNear(World *world, EntityPlayer *player, bool behind);
    void sendHerobrineChat(const std::string &text);
    std::string pickAppearanceLine(World *world) const;

private:
    struct SavedBlock
    {
        int_t x, y, z;
        int_t blockId;
        int_t meta;
    };

    struct AltarRitualState
    {
        bool active = false;
        int_t x = 0; // netherrack block position
        int_t y = 0;
        int_t z = 0;
        int_t ticks = 0;
        int_t entityId = -1;
    };

    struct DiggerState
    {
        bool active = false;
        int_t hx = 0, hy = 0, hz = 0;
        int_t dirX = 1, dirZ = 0;
        int_t steps = 0;
        int_t ticks = 0;
        int_t entityId = -1;
        bool warned = false;
    };

    struct DiamondLureState
    {
        bool active = false;
        int_t targetX = 0, targetY = 0, targetZ = 0;
        int_t curY = 0;
        int_t ticks = 0;
        int_t phase = 0; // 0=waiting/digging, 1=waiting at ore, 2=restoring blocks
        int_t entityId = -1;
        std::vector<SavedBlock> dugBlocks;
    };

    std::string getStateFilePath() const;
    std::string getConfigFilePath() const;
    bool hasActiveHerobrine(World *world) const;
    EntityHerobrine *findHerobrineById(World *world, int_t entityId) const;
    bool isAreaSafeFromBuilds(World *world, int_t cx, int_t cy, int_t cz) const;

    // iLoveYou event engines
    void tickAltarRitual(World *world, EntityPlayer *player);
    void breakTorchesInRadius(World *world, int_t cx, int_t cy, int_t cz, int_t radius);
    void deleteAltarBlocks(World *world, int_t nx, int_t ny, int_t nz);

    void spinEventWheel(World *world, EntityPlayer *player);
    bool startDiggerEvent(World *world, EntityPlayer *player);
    void tickDiggerEvent(World *world, EntityPlayer *player);
    void buildDiggerRoom(World *world, int_t cx, int_t cy, int_t cz);

    bool startDiamondEvent(World *world, EntityPlayer *player);
    void tickDiamondEvent(World *world, EntityPlayer *player);

    bool triggerThiefEvent(World *world, EntityPlayer *player);

    // Optional legend extras (non-destructive away from player builds)
    bool triggerSafeLegendExtra(World *world, EntityPlayer *player);
    bool buildSandPyramid(World *world, int_t sx, int_t sy, int_t sz);
    bool stripLeavesFromNaturalTree(World *world, int_t sx, int_t sy, int_t sz);
    bool placeCreepySign(World *world, int_t sx, int_t sy, int_t sz);
    bool placeLoneRedstoneTorch(World *world, int_t sx, int_t sy, int_t sz);
    void playEerieSound(World *world, EntityPlayer *player);

    Minecraft *m_mc;
    bool m_enabled;
    bool m_summoned;
    HerobrineConfig m_config;
    int_t m_altarX;
    int_t m_altarY;
    int_t m_altarZ;
    int_t m_sightings;
    int_t m_mythEvents;
    int_t m_wheelCooldown;
    int_t m_soundCooldown;
    int_t m_extraCooldown;
    AltarRitualState m_altarRitual;
    DiggerState m_digger;
    DiamondLureState m_diamond;
    std::string m_currentWorldDir;
};
