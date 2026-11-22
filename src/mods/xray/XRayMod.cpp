#include "mods/xray/XRayMod.h"

#include "Minecraft.h"
#include "RenderGlobal.h"
#include "net/minecraft/src/EntityPlayerSP.h"
#include "net/minecraft/src/GuiIngame.h"
#include "net/minecraft/src/SoundManager.h"
#include "net/minecraft/src/World.h"
#include "net/minecraft/src/XRayMode.h"

#if PLATFORM_PS2
#include "ps2/input/Ps2PadState.h"
#elif PLATFORM_XBOX
#include "xbox/input/XboxPad.h"
#endif

#if !PLATFORM_PS2 && !PLATFORM_WII && !PLATFORM_XBOX
#include "lwjgl/Keyboard.h"
#endif

XRayMod::XRayMod()
    : m_mc(nullptr)
    , m_lastWorld(nullptr)
    , m_enabled(false)
    , m_comboWasPressed(false)
{
}

void XRayMod::setEnabled(bool enabled)
{
    m_enabled = enabled;
    XRayMode::setComboEnabled(enabled);
    if (enabled)
    {
        m_lastWorld = (m_mc != nullptr) ? m_mc->theWorld : nullptr;
    }
    else
    {
        m_comboWasPressed = false;
        m_lastWorld = nullptr;
        XRayMode::setActive(false);
    }
}

void XRayMod::onInit(Minecraft *mc)
{
    m_mc = mc;
    m_lastWorld = nullptr;
    m_comboWasPressed = false;
    XRayMode::setComboEnabled(m_enabled);
}

void XRayMod::onTick()
{
    if (!m_enabled || m_mc == nullptr || m_mc->theWorld == nullptr || m_mc->thePlayer == nullptr)
    {
        m_comboWasPressed = false;
        if (XRayMode::active())
            XRayMode::reset();
        m_lastWorld = nullptr;
        return;
    }

    if (m_mc->theWorld != m_lastWorld)
    {
        XRayMode::reset();
        m_lastWorld = m_mc->theWorld;
        m_comboWasPressed = false;
    }

    if (m_mc->currentScreen != nullptr)
    {
        m_comboWasPressed = false;
        return;
    }

    bool comboPressedNow = false;

#if PLATFORM_PS2
    const unsigned short xrayMask = PS2_PAD_L1 | PS2_PAD_R1 | PS2_PAD_L2 | PS2_PAD_R2;
    const Ps2PadSnapshot &pad0 = ps2PadGetSnapshot(0);
    bool combo0 = pad0.connected && (pad0.held & xrayMask) == xrayMask;
    bool combo1 = false;
    if (m_mc->isSplitScreenActive() && m_mc->thePlayer2 != nullptr)
    {
        const Ps2PadSnapshot &pad1 = ps2PadGetSnapshot(1);
        combo1 = pad1.connected && (pad1.held & xrayMask) == xrayMask;
    }
    comboPressedNow = combo0 || combo1;
#elif PLATFORM_XBOX
    const unsigned short xrayMask = XBOX_PAD_WHITE | XBOX_PAD_BLACK | XBOX_PAD_LT | XBOX_PAD_RT;
    const XboxPadSnapshot &pad0 = XboxPad::snapshot();
    bool combo0 = pad0.connected && (pad0.held & xrayMask) == xrayMask;
    bool combo1 = false;
    if (m_mc->isSplitScreenActive() && m_mc->thePlayer2 != nullptr)
    {
        const XboxPadSnapshot &pad1 = XboxPad::playerSnapshot(1);
        combo1 = pad1.connected && (pad1.held & xrayMask) == xrayMask;
    }
    comboPressedNow = combo0 || combo1;
#elif !PLATFORM_WII
    comboPressedNow = lwjgl::Keyboard::isKeyDown(lwjgl::Keyboard::KEY_F6);
#endif

    if (comboPressedNow && !m_comboWasPressed)
    {
        XRayMode::toggle();

        if (m_mc->ingameGUI != nullptr)
            m_mc->ingameGUI->addChatMessage(XRayMode::active()
                ? "\xc2\xa7" "b[X-Ray] ON"
                : "\xc2\xa7" "b[X-Ray] OFF");

        if (m_mc->sndManager != nullptr)
            m_mc->sndManager->playSoundFX("random.click", 1.0f,
                                          XRayMode::active() ? 1.2f : 0.8f);
    }
    m_comboWasPressed = comboPressedNow;

    if (XRayMode::active() && m_mc->renderGlobal != nullptr)
        m_mc->renderGlobal->setAllRenderersVisible();
}
