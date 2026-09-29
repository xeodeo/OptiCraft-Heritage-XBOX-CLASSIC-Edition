#include "MovementInputFromOptions.h"
#include "platform/Log.h"

#include "GameSettings.h"
#include "KeyBinding.h"
#include "Minecraft.h"

#include "platform/Input.h"
#include "platform/PlatformConfig.h"
#include "platform/PlatformTuning.h"

#include "GuiScreen.h"

#ifdef PS2_PLATFORM
#include "ps2/input/Ps2PadState.h"
#endif
#ifdef XBOX_PLATFORM
#include "xbox/input/XboxPad.h"
#include <cmath>
#endif

namespace
{
float clampMovement(float value)
{
    if (value < -1.0f) return -1.0f;
    if (value > 1.0f) return 1.0f;
    return value;
}
}

MovementInputFromOptions::MovementInputFromOptions(GameSettings *gamesettings, int port)
    : gameSettings(gamesettings), padPort(port)
{
}

void MovementInputFromOptions::checkKeyForMovementInput(int i, bool flag)
{
    if (padPort == 0)
        KeyBinding::setKeyBindState(i, flag);
}

void MovementInputFromOptions::resetKeyState()
{
    if (padPort == 0)
        KeyBinding::unPressAllKeys();
}

void MovementInputFromOptions::updatePlayerMoveState(EntityPlayer *entityplayer)
{
    (void)entityplayer;
    moveStrafe = 0.0f;
    moveForward = 0.0f;

    Minecraft *minecraft = Minecraft::getMinecraft();

    if (padPort == 0)
    {
        if (minecraft != nullptr && minecraft->isPlayerScreenActive(0))
        {
            jump = false;
            sneak = false;
        }
        else
        {
            if (gameSettings->keyBindForward->pressed) moveForward++;
            if (gameSettings->keyBindBack->pressed) moveForward--;
            if (gameSettings->keyBindLeft->pressed) moveStrafe++;
            if (gameSettings->keyBindRight->pressed) moveStrafe--;
            jump = gameSettings->keyBindJump->pressed;
            sneak = gameSettings->keyBindSneak->pressed;
        }
    }
    else
    {
#ifdef PS2_PLATFORM
        const Ps2PadSnapshot &ps2Snap = ps2PadGetSnapshot(1);
        if (ps2Snap.connected && !(minecraft != nullptr && minecraft->isPlayerScreenActive(1)))
        {
            jump = (ps2Snap.held & PS2_PAD_CROSS) != 0;
            sneak = (ps2Snap.held & PS2_PAD_R3) != 0;
        }
        else
        {
            jump = false;
            sneak = false;
        }
#elif defined(XBOX_PLATFORM)
        // Split screen's player 2 (XboxSplitScreen): left stick moves, A
        // jumps, the left stick click sneaks, as for player 1. Paused only
        // while player 2 has its own screen open (Legacy-style split screen).
        const XboxPadSnapshot &pad2 = XboxPad::playerSnapshot(1);
        Minecraft *owner = Minecraft::getMinecraft();
        const bool inScreen = owner != nullptr && owner->player2Screen() != nullptr;
        if (pad2.connected && !inScreen)
        {
            static constexpr float kDeadzone = 0.24f;
            auto axis = [](float v) {
                if (v > -kDeadzone && v < kDeadzone) return 0.0f;
                const float sign = v < 0.0f ? -1.0f : 1.0f;
                return sign * (std::fabs(v) - kDeadzone) / (1.0f - kDeadzone);
            };
            moveStrafe -= axis(pad2.leftX);
            moveForward -= axis(pad2.leftY);
            if (pad2.held & XBOX_PAD_DPAD_UP) moveForward++;
            if (pad2.held & XBOX_PAD_DPAD_DOWN) moveForward--;
            if (pad2.held & XBOX_PAD_DPAD_LEFT) moveStrafe++;
            if (pad2.held & XBOX_PAD_DPAD_RIGHT) moveStrafe--;
            jump = (pad2.held & XBOX_PAD_A) != 0;
            sneak = (pad2.held & XBOX_PAD_LEFT_THUMB) != 0;
        }
        else
        {
            jump = false;
            sneak = false;
        }
#else
        jump = false;
        sneak = false;
#endif
    }

#if PLATFORM_DIRECT_ANALOG_MOVEMENT
    const PlatformGamepadSnapshot pad = platformGamepadSnapshot(padPort);
    bool gameplayInput = true;
    if (minecraft != nullptr && minecraft->isPlayerScreenActive(padPort))
    {
        gameplayInput = false;
    }
    else if (minecraft != nullptr && minecraft->currentScreen != nullptr)
    {
        if (minecraft->currentScreen->doesGuiPauseGame())
        {
            gameplayInput = false;
        }
        else
        {
            if (padPort == 0 && !minecraft->isScreenOwnedByPlayer2())
                gameplayInput = false;
            else if (padPort == 1 && minecraft->isScreenOwnedByPlayer2())
                gameplayInput = false;
        }
    }

    if (pad.connected && gameplayInput)
    {
        // PlatformGamepadSnapshot normalizes axes. Minecraft wants positive
        // forward and positive strafe-left, matching the W/A/S/D states.
        moveStrafe += -pad.leftX * PLATFORM_ANALOG_MOVE_SCALE;
        moveForward += -pad.leftY * PLATFORM_ANALOG_MOVE_SCALE;
        moveStrafe = clampMovement(moveStrafe);
        moveForward = clampMovement(moveForward);

#if MC_LOG_LEVEL >= 2
        if (pad.leftX != 0.0f || pad.leftY != 0.0f)
        {
            static int moveLogFrames = 0;
            if (++moveLogFrames >= 30)
            {
                moveLogFrames = 0;
                MC_LOG_DEBUG("input", "move input (pad %d) L=%.3f,%.3f -> strafe=%.3f forward=%.3f jump=%d sneak=%d\n",
                             padPort, pad.leftX, pad.leftY, moveStrafe, moveForward, jump ? 1 : 0, sneak ? 1 : 0);
            }
        }
#endif
    }
#endif

    if (sneak)
    {
        moveStrafe *= 0.29999999999999999;
        moveForward *= 0.29999999999999999;
    }
}
