#pragma once

#include <string>
#include <atomic>
#include <vector>

#include "java/Type.h"
#include "java/String.h"
#include "java/System.h"
#include "platform/PlatformConfig.h"
#include "platform/PlatformTuning.h"

#if !PLATFORM_PS2
struct SDL_Window;
#endif

class File;

class PlayerController;
class World;
class WorldSettings;
class RenderGlobal;
class EntityPlayerSP;
class EntityLiving;
class EffectRenderer;
class RenderEngine;
class FontRenderer;
class GuiScreen;
class LoadingScreenRenderer;
class EntityRenderer;
class GuiAchievement;
class GuiIngame;
class ModelBiped;
class MovingObjectPosition;
class GameSettings;
class SoundManager;
class MouseHelper;
class TexturePackList;
class ISaveFormat;
class StatFileWriter;
class TextureWaterFX;
class TextureLavaFX;
class ThreadDownloadResources;
class ThreadSleepForever;
class NetClientHandler;
class Session;
class Timer;
class UnexpectedThrowable;

class Minecraft;
namespace LegacyStartup
{
    void run(Minecraft* minecraft);
}

// net.minecraft.client.Minecraft + net.minecraft.src.MinecraftImpl (merged)
class Minecraft
{
public:
    struct WorldLoadOptions
    {
        bool naturalMobSpawningEnabled = true;
    };

    Minecraft(int_t width, int_t height, bool flag);
    ~Minecraft();

    static void start(const jstring *username, const jstring *sessionId);
    static Minecraft *getMinecraft();

    void run();
    void startGame();
    void runTick();
    void shutdown();

    void displayGuiScreen(GuiScreen *guiscreen);
    void scheduleGuiScreenDeletion(GuiScreen *screen);
    void purgeOwnedGuiScreens(); // free abandoned menu screens the Java GC would have collected
    void displayInGameMenu();
    void setIngameFocus();
    void setIngameNotInFocus();
    void toggleFullscreen();
    void refreshResources();
    FontRenderer *getStandardGalacticFontRenderer();

    void onMinecraftCrash(UnexpectedThrowable *unexpectedthrowable);
    virtual void displayUnexpectedThrowable(UnexpectedThrowable *unexpectedthrowable);

    void setServer(const std::string &s, int_t i);

    ISaveFormat *getSaveLoader();

    bool isMultiplayerWorld();
    bool isDebugKeepInventoryEnabled();

    void startWorld(const std::string &s, const std::string &s1, long_t l);
    void startWorld(const std::string &s, const std::string &s1, WorldSettings *settings);
    void startWorld(ISaveFormat *saveFormat, const std::string &s, const std::string &s1, WorldSettings *settings);
    void startWorld(ISaveFormat *saveFormat, const std::string &s, const std::string &s1, WorldSettings *settings,
                    const WorldLoadOptions &options);
    void usePortal(int_t targetDimension);
    void changeWorld1(World *world);
    void changeWorld2(World *world, const std::string &s);
    void changeWorld(World *world, const std::string &s, EntityPlayerSP *entityplayer);
    void respawn(bool flag, int_t i, bool copyPlayer = false);

    void installResource(const std::string &s, const std::string &file);

    std::string getDebugLine1(); // func_6241_m
    std::string getDebugLine2(); // func_6262_n
    std::string getDebugLine3(); // func_21002_o
    std::string getDebugLine4(); // func_6245_o

    static bool isGuiEnabled();
    static bool isFancyGraphicsEnabled();
    static bool isAmbientOcclusionEnabled();
    static bool isDebugInfoEnabled();

    bool lineIsCommand(const std::string &s);

    static File *getMinecraftDir();

    NetClientHandler *getSendQueue();

    bool isScreenOwnedByPlayer2() const;
    void setScreenOwnedByPlayer2(bool val);
    bool isSplitScreenActive() const;
    // Split screen, Legacy style: each player has its own screen. The one of
    // the player that is not "in context" waits in otherPlayerScreen. Entering
    // player 2's context swaps it (and the owned-screen list, thePlayer,
    // objectMouseOver and the menu controller) in, so every screen class works
    // for player 2 unchanged: displayGuiScreen/currentScreen then mean P2's.
    void enterPlayer2Context();
    void leavePlayer2Context();
    bool inPlayer2Context() const { return screenPlayer == 1; }
    GuiScreen *player1Screen() const { return screenPlayer == 1 ? otherPlayerScreen : currentScreen; }
    GuiScreen *player2Screen() const { return screenPlayer == 1 ? currentScreen : otherPlayerScreen; }
    // Closes player 2's screen without running its close hooks against
    // player 1 (world change, player 2 leaving).
    void discardPlayer2Screen();
    // GUI size of one screen: half height while two players share the display.
    void guiScreenResolution(int_t &width, int_t &height);
    // Resize both players' screens after split screen starts or stops.
    void refreshScreenResolutions();
    // Split screen: player 2 keeps its own attack/use cooldowns (swapped in
    // with its context); its tick counts them down here.
    void tickClickCounters();
    bool useItemReady() const { return rightClickDelayTimer == 0; }
    // Player 2 is holding RT on a block: player 1's idle "not attacking"
    // tick must not reset the shared block-breaking progress.
    bool player2Mining = false;
    void setSplitScreenActive(bool val);

    void displayPlayerScreen(int playerIndex, GuiScreen *screen);
    GuiScreen *getPlayerScreen(int playerIndex) const;
    void closePlayerScreen(int playerIndex);
    bool isPlayerScreenActive(int playerIndex) const;
    float getPlayerCursorX(int playerIndex) const;
    float getPlayerCursorY(int playerIndex) const;
    void setPlayerCursor(int playerIndex, float x, float y);
    void resetPlayerCursor(int playerIndex, float defaultX, float defaultY);

    void clickMouse(int_t i, bool flag);
    void clickMouse(int_t i);
    void clickMiddleMouseButton();

    // Static fields
    // Vanilla reserves 10 MB so the out-of-memory crash handler still has heap
    // to build the crash screen. Here this storage is static BSS, therefore the
    // memset in freeMemoryForCrash() never releases it. On fixed-memory consoles
    // it only inflates the DOL and steals MEM1 before the allocator starts.
    // Keep a token buffer for the legacy call site, not a fake 10 MB reserve.
#if PLATFORM_HAS_LIMITED_MEMORY
    static byte_t field_28006_b[1];
#else
    static byte_t field_28006_b[0xa00000];
#endif
    static long_t frameTimes[512];
    static long_t tickTimes[512];
    static int_t numRecordedFrameTimes;
#if PLATFORM_CLIENT_PAID_CHECK
    static std::atomic<long_t> hasPaidCheckTime;
#else
    static long_t hasPaidCheckTime;
#endif

    // Public fields
    PlayerController *playerController;
    int_t displayWidth;
    int_t displayHeight;
    World *theWorld;
    RenderGlobal *renderGlobal;
    EntityPlayerSP *thePlayer;
    EntityPlayerSP *thePlayerOne;
    EntityPlayerSP *thePlayer2;
    EntityLiving *renderViewEntity;
    EffectRenderer *effectRenderer;
    Session *session;
    std::string minecraftUri;
    bool hideQuitButton;
    volatile bool isGamePaused;
    RenderEngine *renderEngine;
    FontRenderer *fontRenderer;
    FontRenderer *standardGalacticFontRenderer;
    GuiScreen *currentScreen;
    std::vector<GuiScreen *> ownedGuiScreens;    // every screen we showed; purged when the stack is abandoned
    std::vector<GuiScreen *> guiScreensToDelete; // deferred frees; flushed at runTick start (never delete a screen mid-handler)
    std::vector<World *> worldsToDelete;         // deferred world frees; flushed at runTick start (changeWorld can run mid-tick via MP packet handling -> never delete the world being ticked)
    LoadingScreenRenderer *loadingScreen;
    EntityRenderer *entityRenderer;
    int_t ticksRan;
    GuiAchievement *guiAchievement;
    GuiIngame *ingameGUI;
    bool skipRenderWorld;
    ModelBiped *field_9242_w;
    MovingObjectPosition *objectMouseOver;
    MovingObjectPosition *objectMouseOver2;
    GameSettings *gameSettings;
    SoundManager *sndManager;
    MouseHelper *mouseHelper;
    TexturePackList *texturePackList;
    StatFileWriter *statFileWriter;
    volatile bool running;
    std::string debug;
    // Frame-time split around the buffer swap (lwjgl::Display::update()):
    // gpuUsagePercent is the share of the frame spent waiting on that swap
    // (vsync/present, a stand-in for GPU-bound time on platforms with no real
    // GPU utilization counter), cpuUsagePercent is the rest (tick + render
    // submission). Updated once a second, alongside `debug`. Shown on F3.
    float cpuUsagePercent;
    float gpuUsagePercent;
    bool inGameHasFocus;
    bool isRaining;
    bool screenOwnedByPlayer2;
    bool splitScreenActive;
    GuiScreen *otherPlayerScreen = nullptr;
    std::vector<GuiScreen *> otherOwnedGuiScreens;
    int_t screenPlayer = 0;          // 0: player 1 in context, 1: player 2
    EntityPlayerSP *contextSavedPlayer = nullptr;
    int_t otherLeftClickCounter = 0;
    int_t otherRightClickDelayTimer = 0;
    GuiScreen *playerScreens[2];
    float playerCursorX[2];
    float playerCursorY[2];
    bool playerCursorInitialized[2];
    int ignorePauseMenuTicks;
#if !PLATFORM_PS2
    SDL_Window *window;
#endif

private:
    friend void LegacyStartup::run(Minecraft* minecraft);

    void loadScreen();
    void drawSplashScreenTexturedModalRect(int_t i, int_t j, int_t k, int_t l, int_t i1, int_t j1);
    void checkGLError(const std::string &s);
    void shutdownMinecraftApplet();
    void screenshotListener();
    void displayDebugInfo(long_t l);
    void startCheckHasPaidThread();
    void freeMemoryForCrash();
    void forceReload();
    void convertMapFormat(const std::string &s, const std::string &s1);
    // radiusBlocks < 0: PLATFORM_PRELOAD_RADIUS_BLOCKS (see preloadWorld).
    void preloadWorld(const std::string &s, int_t radiusBlocks = -1);
    void resize(int_t i, int_t j);

    static File *getAppDir(const std::string &s);
    static Minecraft *theMinecraft;
    static File *minecraftDir;

    bool fullscreen;
    bool hasCrashed;
    Timer *timer;
    ThreadDownloadResources *downloadResourcesThread;
    ThreadSleepForever *timerHackThread;
    int_t leftClickCounter;
    int_t rightClickDelayTimer;
    int_t tempDisplayWidth;
    int_t tempDisplayHeight;
    File *mcDataDir;
    ISaveFormat *saveLoader;
    std::string serverName;
    int_t serverPort;
    TextureWaterFX *textureWaterFX;
    TextureLavaFX *textureLavaFX;
    bool isTakingScreenshot;
    long_t prevFrameTime;
    int_t mouseTicksRan;
    long_t systemTime;
    int_t joinPlayerCounter;
};
