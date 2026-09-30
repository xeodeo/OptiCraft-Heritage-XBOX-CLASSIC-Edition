// XboxSplitScreen.cpp â€” two-player split screen on the Xbox.
//
// Follows upstream's PS2 split screen (Ps2SplitScreen.cpp): player 2 joins
// with START on the second controller, only in Limited (256x256) worlds so
// both views stay inside the loaded map, and the two players are tethered
// within 32 blocks. Player 2 uses the same layout as player 1:
//   left stick move, right stick look, A jump, LS sneak, RT attack/mine,
//   LT use/place, White/Black hotbar, Y inventory, B drop, START pause.
// Each player has its own screens, as on the Legacy editions: player 2's
// inventory, crafting, containers and pause menu run in its own context
// (Minecraft::enterPlayer2Context) and draw in its half of the display, and
// the other player keeps playing meanwhile. Player 2's menus use the D-pad
// (or left stick); the stick pointer stays player 1's.
//
// Lives in src/client, not src/xbox: sources there get the XDK include path
// and its old STL, which must not mix with the game's (see JavaNetworkXbox).
#include "client/XboxSplitScreen.h"

#ifdef XBOX_PLATFORM

#include "client/Minecraft.h"
#include "net/minecraft/src/EntityPlayerSP.h"
#include "net/minecraft/src/World.h"
#include "net/minecraft/src/WorldInfo.h"
#include "net/minecraft/src/WorldProvider.h"
#include "net/minecraft/src/Session.h"
#include "net/minecraft/src/MovementInputFromOptions.h"
#include "net/minecraft/src/skin/SkinManager.h"
#include "net/minecraft/src/GuiInventory.h"
#include "net/minecraft/src/GuiIngame.h"
#include "net/minecraft/src/InventoryPlayer.h"
#include "net/minecraft/src/FoodStats.h"
#include "net/minecraft/src/ChunkCoordinates.h"
#include "net/minecraft/src/PlayerController.h"
#include "net/minecraft/src/SoundManager.h"
#include "net/minecraft/src/StringTranslate.h"
#include "net/minecraft/src/GameSettings.h"
#include "net/minecraft/src/MovingObjectPosition.h"
#include "net/minecraft/src/GuiScreen.h"
#include "net/minecraft/src/GuiContainerCreative.h"
#include "net/minecraft/src/legacy/LegacyCraftingScreen.h"
#include "net/minecraft/src/legacy/XboxCraftingScreen.h"
#include "pc/lwjgl/Keyboard.h"
#include "xbox/input/XboxPad.h"
#include "platform/Log.h"

#include <cmath>

namespace XboxSplitScreen
{
namespace
{
constexpr float kTetherDistance = 32.0f;
// Same as player 1's path: stick -> 14 "mouse pixels" a frame
// (XboxInput CAM_SCALE) -> MouseHelper sensitivity curve.
constexpr float kCameraPixels = 14.0f;
constexpr float kStickDeadzone = 0.24f;
Session s_sessionP2("Player 2", "");
bool s_leaveRequested = false;
// Left stick as a D-pad in player 2's menus: edge, then repeat while held.
int s_stickDirection = 0;
int s_stickRepeat = 0;
constexpr int kStickRepeatFirst = 6;   // ticks
constexpr int kStickRepeatNext = 2;

bool isSpanish()
{
    StringTranslate *tr = StringTranslate::getInstance();
    return tr != nullptr && tr->getCurrentLanguage().rfind("es_", 0) == 0;
}

void chat(Minecraft *mc, const char *es, const char *en)
{
    if (mc->ingameGUI != nullptr)
        mc->ingameGUI->addChatMessage(isSpanish() ? es : en);
}

float deadzone(float v)
{
    if (v > -kStickDeadzone && v < kStickDeadzone)
        return 0.0f;
    const float sign = v < 0.0f ? -1.0f : 1.0f;
    return sign * (std::fabs(v) - kStickDeadzone) / (1.0f - kStickDeadzone);
}

// Runs `open` as player 2: displayGuiScreen then targets player 2's screen.
template <typename F>
void asPlayer2(Minecraft *mc, F open)
{
    mc->enterPlayer2Context();
    open();
    mc->leavePlayer2Context();
}

// Stick direction as a D-pad mask for player 2's menus.
unsigned short stickAsDpad(const XboxPadSnapshot &pad)
{
    const float x = pad.leftX, y = pad.leftY;
    int dir = 0;
    if (std::fabs(x) >= 0.6f || std::fabs(y) >= 0.6f)
        dir = std::fabs(y) >= std::fabs(x) ? (y < 0.0f ? 1 : 2) : (x < 0.0f ? 3 : 4);
    else if (s_stickDirection != 0 && (std::fabs(x) > 0.35f || std::fabs(y) > 0.35f))
        dir = s_stickDirection;
    if (dir != s_stickDirection)
    {
        s_stickDirection = dir;
        s_stickRepeat = kStickRepeatFirst;
    }
    else if (dir != 0 && --s_stickRepeat <= 0)
        s_stickRepeat = kStickRepeatNext;
    else
        return 0;
    switch (dir)
    {
        case 1: return XBOX_PAD_DPAD_UP;
        case 2: return XBOX_PAD_DPAD_DOWN;
        case 3: return XBOX_PAD_DPAD_LEFT;
        case 4: return XBOX_PAD_DPAD_RIGHT;
        default: return 0;
    }
}
} // namespace

void joinPlayer2(Minecraft *mc)
{
    if (mc == nullptr || mc->theWorld == nullptr || mc->thePlayer == nullptr || mc->thePlayer2 != nullptr)
        return;
    if (mc->theWorld->multiplayerWorld)
        return;

    WorldInfo *worldInfo = mc->theWorld->getWorldInfo();
    if (worldInfo == nullptr || !worldInfo->isLimitedWorld())
    {
        MC_LOG_INFO("split", "player 2 join refused: world is not Limited\n");
        chat(mc, "\xc2\xa7" "c[Pantalla dividida] Solo en mundos Limited (256x256): World Size al crear el mundo.",
                 "\xc2\xa7" "c[Split screen] Only in Limited (256x256) worlds: World Size when creating the world.");
        return;
    }

    mc->thePlayerOne = mc->thePlayer;
    // Player 2's own name (Options > Heritage > Player 2 Name).
    if (mc->gameSettings != nullptr && !mc->gameSettings->playerName2.empty())
        s_sessionP2.username = mc->gameSettings->playerName2;

    EntityPlayerSP *p2 = new EntityPlayerSP(mc, mc->theWorld, &s_sessionP2, mc->theWorld->worldProvider->worldType);
    delete p2->movementInput;
    p2->movementInput = new MovementInputFromOptions(mc->gameSettings, 1);

    const std::string skinP2 = SkinManager::getPlayer2SkinTexture();
    if (!skinP2.empty())
    {
        p2->skinUrl = "";
        p2->setEntityTexture(skinP2);
    }
    p2->setLocationAndAngles(mc->thePlayer->posX + 1.0, mc->thePlayer->posY, mc->thePlayer->posZ + 1.0,
                             mc->thePlayer->rotationYaw, mc->thePlayer->rotationPitch);
    p2->capabilities = mc->thePlayer->capabilities;
    mc->theWorld->spawnEntityInWorld(p2);

    mc->thePlayer2 = p2;
    mc->setSplitScreenActive(true);
    mc->refreshScreenResolutions();   // player 1's open screen moves to its half
    MC_LOG_INFO("split", "player 2 joined\n");
    if (mc->sndManager != nullptr)
        mc->sndManager->playSoundFX("random.levelup", 1.0f, 1.0f);
    chat(mc, "\xc2\xa7" "a[J2] Jugador 2 conectado!", "\xc2\xa7" "a[P2] Player 2 connected!");
    chat(mc, "\xc2\xa7" "b[J2] Back: salir de la partida", "\xc2\xa7" "b[P2] Back: leave the game");
}

void leavePlayer2(Minecraft *mc)
{
    if (mc == nullptr)
        return;
    mc->leavePlayer2Context();
    mc->discardPlayer2Screen();
    if (mc->thePlayer2 != nullptr)
    {
        if (mc->theWorld != nullptr)
            mc->theWorld->detachEntityForWorldChange(mc->thePlayer2);
        delete mc->thePlayer2;
        mc->thePlayer2 = nullptr;
    }
    delete mc->objectMouseOver2;
    mc->objectMouseOver2 = nullptr;
    if (mc->thePlayerOne != nullptr)
        mc->thePlayer = mc->thePlayerOne;
    mc->setSplitScreenActive(false);
    mc->setScreenOwnedByPlayer2(false);
    XboxPad::setMenuPlayer(0);
    s_leaveRequested = false;
    mc->refreshScreenResolutions();   // player 1's open screen gets the full display again
    MC_LOG_INFO("split", "player 2 left\n");
}

void tick(Minecraft *mc)
{
    if (mc == nullptr)
        return;

    const XboxPadSnapshot &pad2 = XboxPad::playerSnapshot(1);
    static unsigned short s_prevHeld = 0;
    const unsigned short held = pad2.connected ? pad2.held : 0;
    const unsigned short pressed = static_cast<unsigned short>(held & ~s_prevHeld);
    const unsigned short released = static_cast<unsigned short>(s_prevHeld & ~held);
    s_prevHeld = held;
#if MC_LOG_LEVEL > 0
    if (pressed != 0)
        MC_LOG_INFO("split", "pad 2 pressed %04x (screen=%d world=%d p2=%d)\n", pressed,
                    mc->currentScreen != nullptr ? 1 : 0, mc->theWorld != nullptr ? 1 : 0,
                    mc->thePlayer2 != nullptr ? 1 : 0);
#endif

    if (mc->thePlayer2 == nullptr)
    {
        if ((pressed & XBOX_PAD_START) && mc->currentScreen == nullptr && mc->theWorld != nullptr &&
            !mc->theWorld->multiplayerWorld && mc->thePlayer != nullptr)
            joinPlayer2(mc);
        return;
    }

    // Controller unplugged, Back while playing, or "Leave Game": player 2 leaves.
    if (!pad2.connected || s_leaveRequested ||
        ((pressed & XBOX_PAD_BACK) && mc->player2Screen() == nullptr))
    {
        leavePlayer2(mc);
        chat(mc, "\xc2\xa7" "e[J2] Jugador 2 salio.", "\xc2\xa7" "e[P2] Player 2 left.");
        return;
    }

    EntityPlayerSP *p2 = mc->thePlayer2;

    // Player 2 respawns next to player 1.
    if (p2->isDead || p2->getHealth() <= 0)
    {
        p2->isDead = false;
        p2->deathTime = 0;
        p2->setHealth(20);
        if (p2->getFoodStats() != nullptr)
        {
            p2->getFoodStats()->setFoodLevel(20);
            p2->getFoodStats()->setFoodSaturationLevel(5.0f);
        }
        p2->clearActivePotions();
        if (mc->thePlayerOne != nullptr && !mc->thePlayerOne->isDead)
            p2->setLocationAndAngles(mc->thePlayerOne->posX + 1.0, mc->thePlayerOne->posY,
                                     mc->thePlayerOne->posZ + 1.0, 0.0f, 0.0f);
        else if (mc->theWorld != nullptr)
        {
            ChunkCoordinates spawn = mc->theWorld->getSpawnPoint();
            p2->setLocationAndAngles(spawn.x + 0.5, spawn.y + 1.0, spawn.z + 0.5, 0.0f, 0.0f);
        }
        // onDeath() shrank the hitbox to 0.2x0.2; without the vanilla spawn
        // reset the revived P2 keeps it and player 1's hits pass through.
        p2->preparePlayerToSpawn();
        p2->hurtTime = 0;
        p2->heartsLife = 0;
        p2->motionX = p2->motionY = p2->motionZ = 0.0;
        return;
    }

    if (p2->inventory != nullptr)
    {
        if (pressed & XBOX_PAD_BLACK)
            p2->inventory->currentItem = (p2->inventory->currentItem + 1) % 9;
        if (pressed & XBOX_PAD_WHITE)
            p2->inventory->currentItem = (p2->inventory->currentItem + 8) % 9;
    }

    // Player 2's own screen: runs in its context; player 1 is untouched.
    if (mc->player2Screen() != nullptr)
    {
        const unsigned short stick = stickAsDpad(pad2);
        mc->enterPlayer2Context();
        if (stick != 0)
            XboxPad::latchPressed(stick);
        GuiScreen *screen = mc->currentScreen;
        // Screens driven by the pointer/keyboard path close on Escape; player 1
        // gets it from XboxInput, player 2 from here.
        if (screen != nullptr && !screen->usesSpecializedMenuNavigationForPlatform() &&
            (pressed & (XBOX_PAD_B | XBOX_PAD_Y)) != 0)
            screen->injectKeyTyped(0, lwjgl::Keyboard::KEY_ESCAPE);
        if (mc->currentScreen != nullptr)
            mc->currentScreen->handleInput();
        if (mc->currentScreen != nullptr)
            mc->currentScreen->updateScreen();
        mc->leavePlayer2Context();
        return;
    }
    // Presses made while playing must not click in the next menu.
    XboxPad::clearLatchedPressed(1);
    s_stickDirection = 0;

    if (pressed & XBOX_PAD_START)
    {
        asPlayer2(mc, [mc]() { mc->displayInGameMenu(); });
        return;
    }
    if (pressed & XBOX_PAD_Y)
    {
        asPlayer2(mc, [mc, p2]() {
            if (mc->playerController != nullptr && mc->playerController->isInCreativeMode())
                mc->displayGuiScreen(new GuiContainerCreative(p2));
            else
                mc->displayGuiScreen(new GuiInventory(p2));
        });
        return;
    }
    // X: console crafting (2x2), as for player 1.
    if ((pressed & XBOX_PAD_X) && mc->gameSettings->xboxStyleCrafting && mc->playerController != nullptr &&
        !mc->playerController->isInCreativeMode())
    {
        asPlayer2(mc, [mc, p2]() { mc->displayGuiScreen(new XboxCraftingScreen(p2)); });
        return;
    }
    if (pressed & XBOX_PAD_B)
        p2->dropOneItem();

    // Attack / use as player 2. A block that opens a screen (workbench,
    // chest...) opens it in player 2's context, so it is player 2's.
    // Same order as player 1's input (Minecraft::runTick): the press is the
    // hit / block click, holding keeps mining, LT uses and repeats, and
    // releasing LT finishes eating, drawing a bow...
    mc->player2Mining = (held & XBOX_PAD_RT) != 0;
    mc->enterPlayer2Context();
    mc->tickClickCounters();
    if (p2->isUsingItem() && (held & XBOX_PAD_LT) == 0 && mc->playerController != nullptr)
        mc->playerController->onStoppedUsingItem(p2);
    if (pressed & XBOX_PAD_RT)
        mc->clickMouse(0);
    if (pressed & XBOX_PAD_LT)
        mc->clickMouse(1);
    else if ((held & XBOX_PAD_LT) && mc->useItemReady() && !p2->isUsingItem())
        mc->clickMouse(1);
    if (held & XBOX_PAD_RT)
        mc->clickMouse(0, true);
    else if ((released & XBOX_PAD_RT) && mc->playerController != nullptr)
        mc->playerController->resetBlockRemoving();
    mc->leavePlayer2Context();
}

void requestLeave()
{
    s_leaveRequested = true;
}

void postTick(Minecraft *mc)
{
    if (mc == nullptr || !mc->isSplitScreenActive() || mc->thePlayer2 == nullptr || mc->thePlayerOne == nullptr)
        return;
    EntityPlayerSP *p1 = mc->thePlayerOne;
    EntityPlayerSP *p2 = mc->thePlayer2;
    if (p1->isDead || p2->isDead)
        return;

    const double dx = p2->posX - p1->posX;
    const double dz = p2->posZ - p1->posZ;
    const double distSq = dx * dx + dz * dz;
    if (distSq <= static_cast<double>(kTetherDistance) * kTetherDistance)
        return;
    const double dist = std::sqrt(distSq);
    if (dist < 1e-4)
        return;
    const double nx = dx / dist;
    const double nz = dz / dist;

    // Cancel only the separating part of each player's motion, then put the
    // one moving away back on the tether circle.
    const double p2Out = p2->motionX * nx + p2->motionZ * nz;
    const double p1Out = -p1->motionX * nx - p1->motionZ * nz;
    if (p2Out > 0.0)
    {
        p2->motionX -= p2Out * nx;
        p2->motionZ -= p2Out * nz;
    }
    if (p1Out > 0.0)
    {
        p1->motionX += p1Out * nx;
        p1->motionZ += p1Out * nz;
    }
    if (p2Out >= p1Out)
        p2->setPosition(p1->posX + nx * kTetherDistance, p2->posY, p1->posZ + nz * kTetherDistance);
    else
        p1->setPosition(p2->posX - nx * kTetherDistance, p1->posY, p2->posZ - nz * kTetherDistance);
}

void turnCamera(Minecraft *mc)
{
    if (mc == nullptr || !mc->isSplitScreenActive() || mc->thePlayer2 == nullptr || mc->player2Screen() != nullptr)
        return;
    const XboxPadSnapshot &pad2 = XboxPad::playerSnapshot(1);
    if (!pad2.connected)
        return;
    const float rx = deadzone(pad2.rightX);
    const float ry = deadzone(pad2.rightY);
    if (rx == 0.0f && ry == 0.0f)
        return;
    const float sensitivity = mc->gameSettings->mouseSensitivity * 0.6f + 0.2f;
    const float scale = sensitivity * sensitivity * sensitivity * 8.0f * kCameraPixels;
    const int invert = mc->gameSettings->invertMouse ? -1 : 1;
    // Stick down is positive; the mouse path turns that into looking down.
    mc->thePlayer2->turnEntity(rx * scale, -ry * scale * static_cast<float>(invert));
}

} // namespace XboxSplitScreen

#endif // XBOX_PLATFORM
