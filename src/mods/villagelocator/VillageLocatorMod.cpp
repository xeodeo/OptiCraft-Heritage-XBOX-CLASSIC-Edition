#include "mods/villagelocator/VillageLocatorMod.h"

#include "Minecraft.h"
#include "net/minecraft/src/ChunkPosition.h"
#include "net/minecraft/src/EntityPlayerSP.h"
#include "net/minecraft/src/GuiIngame.h"
#include "net/minecraft/src/SoundManager.h"
#include "net/minecraft/src/World.h"

#include <cmath>
#include <cstdio>

#if PLATFORM_PS2
#include "ps2/input/Ps2PadState.h"
#elif PLATFORM_XBOX
#include "xbox/input/XboxInput.h"
#endif

#if !PLATFORM_PS2 && !PLATFORM_WII && !PLATFORM_XBOX
#include "lwjgl/Keyboard.h"
#endif

VillageLocatorMod::VillageLocatorMod()
    : m_mc(nullptr)
    , m_enabled(false)
    , m_comboWasPressed(false)
{
}

void VillageLocatorMod::setEnabled(bool enabled)
{
    m_enabled = enabled;
    if (!enabled)
    {
        m_comboWasPressed = false;
#if PLATFORM_XBOX
        (void)XboxInput::consumeVillageLocatorRequest();
#endif
    }
}

void VillageLocatorMod::onInit(Minecraft *mc)
{
    m_mc = mc;
    m_comboWasPressed = false;
}

void VillageLocatorMod::onTick()
{
    if (!m_enabled || m_mc == nullptr || m_mc->theWorld == nullptr || m_mc->thePlayer == nullptr)
    {
        m_comboWasPressed = false;
        return;
    }

    if (m_mc->currentScreen != nullptr)
    {
        m_comboWasPressed = false;
        return;
    }

#if PLATFORM_XBOX
    // Back+Y on Xbox (consumed cleanly via XboxInput so releasing Back does not open F3)
    if (XboxInput::consumeVillageLocatorRequest())
    {
        triggerLocator(m_mc->thePlayer);
    }
    return;
#else
    bool comboPressedNow = false;
#if PLATFORM_PS2
    const unsigned short villageMask = PS2_PAD_TRIANGLE | PS2_PAD_SELECT;
    const Ps2PadSnapshot &pad0 = ps2PadGetSnapshot(0);
    const bool combo0 = pad0.connected && (pad0.held & villageMask) == villageMask;
    bool combo1 = false;
    if (m_mc->isSplitScreenActive() && m_mc->thePlayer2 != nullptr)
    {
        const Ps2PadSnapshot &pad1 = ps2PadGetSnapshot(1);
        combo1 = pad1.connected && (pad1.held & villageMask) == villageMask;
    }
    comboPressedNow = combo0 || combo1;
#elif !PLATFORM_WII
    comboPressedNow = lwjgl::Keyboard::isKeyDown(lwjgl::Keyboard::KEY_F8);
#endif

    if (comboPressedNow && !m_comboWasPressed)
        triggerLocator(m_mc->thePlayer);

    m_comboWasPressed = comboPressedNow;
#endif
}

void VillageLocatorMod::triggerLocator(EntityPlayerSP *player)
{
    if (m_mc == nullptr || m_mc->theWorld == nullptr || player == nullptr)
        return;

    const int_t px = static_cast<int_t>(std::floor(player->posX));
    const int_t py = static_cast<int_t>(std::floor(player->posY));
    const int_t pz = static_cast<int_t>(std::floor(player->posZ));

    ChunkPosition *pos = m_mc->theWorld->findClosestStructure("Village", px, py, pz);
    if (pos != nullptr)
    {
        const int_t dx = pos->x - px;
        const int_t dz = pos->z - pz;
        const int_t adx = dx < 0 ? -dx : dx;
        const int_t adz = dz < 0 ? -dz : dz;
        const int_t dist = static_cast<int_t>(std::sqrt(
            static_cast<double>(dx) * static_cast<double>(dx) +
            static_cast<double>(dz) * static_cast<double>(dz)));

        const char *direction;
        if (adx >= adz)
            direction = dx >= 0 ? "Este" : "Oeste";
        else
            direction = dz >= 0 ? "Sur" : "Norte";

        char buf[192];
        std::snprintf(buf, sizeof(buf),
            "\xc2\xa7" "e[Village] X: %d, Y: %d, Z: %d | Dist: %dm | %s",
            static_cast<int>(pos->x),
            static_cast<int>(pos->y),
            static_cast<int>(pos->z),
            static_cast<int>(dist),
            direction);

        if (m_mc->ingameGUI != nullptr)
            m_mc->ingameGUI->addChatMessage(buf);

        if (m_mc->sndManager != nullptr)
            m_mc->sndManager->playSoundFX("random.click", 1.0f, 1.2f);

        delete pos;
    }
    else
    {
        if (m_mc->ingameGUI != nullptr)
            m_mc->ingameGUI->addChatMessage("\xc2\xa7" "c[Village] No se encontro ninguna aldea cerca.");
    }
}
