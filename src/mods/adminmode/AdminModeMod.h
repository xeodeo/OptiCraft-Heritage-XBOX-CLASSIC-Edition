#pragma once

#include "mods/IMod.h"
#include "mods/adminmode/AdminHooks.h"

#include <string>
#include <vector>

class Minecraft;
class World;
class Entity;
class EntityLiving;
class EntityPlayer;
class EntityPlayerSP;

// "Admin mode (testing)" (encargo 122): the Nodus 1.5.2 client modules (Fly,
// NoFall, FullBright, Freecam, Nuker, ESP...) as one catalog mod for testing
// the game with a controller. Works only in a local world / split screen, or
// on a server where the player is op; on someone else's server it does nothing
// and says so. X-Ray and waypoints are the existing X-Ray and Minimap mods.
class AdminModeMod : public IMod
{
public:
    AdminModeMod();
    static AdminModeMod *getInstance();

    std::string getId() const override { return "adminmode"; }
    std::string getName() const override;
    std::string getVersion() const override { return "v1.0"; }
    std::string getDescription() const override;
    std::string getAuthor() const override { return "OptiCraft (Nodus 1.5.2)"; }

    bool isEnabled() const override { return m_enabled; }
    void setEnabled(bool enabled) override;

    bool hasConfigScreen() const override { return true; }
    GuiScreen *createConfigScreen(GuiScreen *parent) override;

    void onInit(Minecraft *mc) override;
    void onTick() override;
    void onRenderGameOverlay(GuiIngame *gui, int_t screenWidth, int_t screenHeight, float_t partialTick) override;

    // Module table used by the menu.
    static int moduleCount() { return AdminHooks::COUNT; }
    static std::string moduleName(int m);
    static std::string moduleDesc(int m);
    static int optionCount(int m);
    static std::string optionLabel(int m, int opt);
    static bool localWorldOnly(int m);

    bool moduleState(int m) const { return m >= 0 && m < AdminHooks::COUNT && m_state[m]; }
    void setModuleState(int m, bool on);
    int moduleOption(int m) const { return (m >= 0 && m < AdminHooks::COUNT) ? m_option[m] : 0; }
    void setModuleOption(int m, int opt);
    void allOff();
    // True when the current world lets the modules run (local world, or op on the server).
    bool worldAllowed() const { return m_allowed; }
    bool inWorld() const;
    void saveConfig();

    // Hook back-ends.
    float highJumpFactor() const;
    bool fastBreakInstant() const { return m_option[AdminHooks::FastBreak] == 1; }
    bool noRenderItems() const { return m_option[AdminHooks::NoRender] == 1; }
    void onBlockPlaced(int x, int y, int z, int side);
    void renderWorldOverlay(float partialTicks);

#if XBOX_AUTOPILOT
    void probeTick(); // PROBE122 (temporary)
#endif
private:
    void loadConfig();
    std::string configPath() const;
    void refreshFlags();
    void onModuleChanged(int m, bool on);
    void restoreModule(int m);
    void handleMenuCombo();
    void openMenu(int playerIndex);

    void tickPlayer(EntityPlayerSP *p, bool primary);
    void tickCombat(EntityPlayerSP *p);
    void tickPrimary(EntityPlayerSP *p);
    void tickNuker(EntityPlayerSP *p, bool attackEdge);
    void tickBlockEsp(EntityPlayerSP *p);
    void tickNotifications(EntityPlayerSP *p);
    void tickAutoEnchant(EntityPlayerSP *p);

    bool isTarget(EntityPlayerSP *p, Entity *e, int mode) const;
    EntityLiving *nearestTarget(EntityPlayerSP *p, double range, int mode, bool needVisible) const;
    void faceEntity(EntityPlayerSP *p, Entity *t, double extraY) const;
    void attack(EntityPlayerSP *p, Entity *t);
    void chat(const std::string &es, const std::string &en);

    Minecraft *m_mc;
    bool m_enabled;
    bool m_allowed;
    bool m_state[AdminHooks::COUNT];
    int m_option[AdminHooks::COUNT];
    bool m_prevOn[AdminHooks::COUNT];
    World *m_lastWorld;
    bool m_blockedWarned;
    bool m_menuComboWasPressed;
    int m_tick;

    // Freecam: where the body stays.
    bool m_freecamSaved;
    double m_fcX, m_fcY, m_fcZ;
    float m_fcYaw, m_fcPitch;
    bool m_fcWasFlying, m_fcAllowFlying, m_fcNoDamage;

    // Input edges (player 1).
    bool m_attackWasDown;
    bool m_useWasDown;
    bool m_autoMineForcing;

    int m_combatCooldown[2];
    int m_fishRecast;
    int m_nukerBlockId;
    int m_enchantCooldown;
    int m_enchantLast[3];
    int m_floodTimer;
    bool m_healthWarned[2];
    float m_derpYaw;

    // BlockESP incremental scan.
    struct BlockPos { int x, y, z; };
    std::vector<BlockPos> m_blockHits;
    std::vector<BlockPos> m_blockScan;
    int m_scanX, m_scanCX, m_scanCY, m_scanCZ, m_scanId;

    // Notifications: players in visual range.
    std::vector<std::string> m_seenPlayers;
};
