#include "mods/strongholdlocator/StrongholdLocatorMod.h"

#include "Minecraft.h"
#include "net/minecraft/src/World.h"
#include "net/minecraft/src/EntityPlayerSP.h"
#include "net/minecraft/src/GuiIngame.h"
#include "net/minecraft/src/SoundManager.h"
#include "net/minecraft/src/ChunkPosition.h"

#include <cmath>
#include <cstdio>
#include <string>

#if PLATFORM_PS2
#include "ps2/input/Ps2PadState.h"
#elif PLATFORM_XBOX
#include "xbox/input/XboxPad.h"
#endif

#if !PLATFORM_PS2 && !PLATFORM_WII && !PLATFORM_XBOX
#include "lwjgl/Keyboard.h"
#endif

StrongholdLocatorMod::StrongholdLocatorMod()
    : m_mc(nullptr)
    , m_enabled(true)
    , m_comboWasPressed(false)
{
}

void StrongholdLocatorMod::onInit(Minecraft *mc)
{
    m_mc = mc;
}

void StrongholdLocatorMod::onTick()
{
    if (!m_enabled || m_mc == nullptr || m_mc->theWorld == nullptr || m_mc->thePlayer == nullptr)
    {
        m_comboWasPressed = false;
        return;
    }

    // Do not trigger while any modal menu or options screen is open
    if (m_mc->currentScreen != nullptr)
    {
        m_comboWasPressed = false;
        return;
    }

    bool comboPressedNow = false;

#if PLATFORM_PS2
    const Ps2PadSnapshot &pad0 = ps2PadGetSnapshot(0);
    bool combo0 = (pad0.held & PS2_PAD_L3) != 0 && (pad0.held & PS2_PAD_R3) != 0;
    bool combo1 = false;
    if (m_mc->isSplitScreenActive() && m_mc->thePlayer2 != nullptr)
    {
        const Ps2PadSnapshot &pad1 = ps2PadGetSnapshot(1);
        combo1 = (pad1.held & PS2_PAD_L3) != 0 && (pad1.held & PS2_PAD_R3) != 0;
    }
    comboPressedNow = (combo0 || combo1);
#elif PLATFORM_XBOX
    const XboxPadSnapshot &pad0 = XboxPad::snapshot();
    bool combo0 = (pad0.held & XBOX_PAD_LEFT_THUMB) != 0 && (pad0.held & XBOX_PAD_RIGHT_THUMB) != 0;
    bool combo1 = false;
    if (m_mc->isSplitScreenActive() && m_mc->thePlayer2 != nullptr)
    {
        const XboxPadSnapshot &pad1 = XboxPad::playerSnapshot(1);
        combo1 = (pad1.held & XBOX_PAD_LEFT_THUMB) != 0 && (pad1.held & XBOX_PAD_RIGHT_THUMB) != 0;
    }
    comboPressedNow = (combo0 || combo1);
#elif !PLATFORM_WII
    comboPressedNow = lwjgl::Keyboard::isKeyDown(lwjgl::Keyboard::KEY_F7);
#endif

    if (comboPressedNow && !m_comboWasPressed)
    {
        triggerLocator();
    }
    m_comboWasPressed = comboPressedNow;
}

void StrongholdLocatorMod::triggerLocator()
{
    if (m_mc == nullptr || m_mc->theWorld == nullptr || m_mc->thePlayer == nullptr)
        return;

    EntityPlayerSP *player = m_mc->thePlayer;
    int_t px = static_cast<int_t>(std::floor(player->posX));
    int_t py = static_cast<int_t>(std::floor(player->posY));
    int_t pz = static_cast<int_t>(std::floor(player->posZ));

    ChunkPosition *pos = m_mc->theWorld->findClosestStructure("Stronghold", px, py, pz);
    if (pos != nullptr)
    {
        int_t dx = pos->x - px;
        int_t dz = pos->z - pz;
        int_t dist = static_cast<int_t>(std::sqrt(static_cast<double>(dx * dx + dz * dz)));

        bool insideMap = true;
        if (m_mc->theWorld->isLimitedWorld())
        {
            insideMap = (pos->x >= -128 && pos->x <= 127 && pos->z >= -128 && pos->z <= 127);
        }

        char buf[160];
        std::snprintf(buf, sizeof(buf),
            "\xc2\xa7" "e[Stronghold] X: %d, Y: %d, Z: %d | Dist: %dm | %s",
            static_cast<int>(pos->x),
            static_cast<int>(pos->y),
            static_cast<int>(pos->z),
            static_cast<int>(dist),
            insideMap ? "\xc2\xa7" "aDENTRO DEL MAPA" : "\xc2\xa7" "cFUERA DEL MAPA");

        if (m_mc->ingameGUI != nullptr)
        {
            m_mc->ingameGUI->addChatMessage(buf);
        }

        if (m_mc->sndManager != nullptr)
        {
            m_mc->sndManager->playSoundFX("random.click", 1.0f, 1.2f);
        }

        delete pos;
    }
    else
    {
        if (m_mc->ingameGUI != nullptr)
        {
            m_mc->ingameGUI->addChatMessage("\xc2\xa7" "c[Stronghold] No se encontro ninguna fortaleza en este mundo.");
        }
    }
}
