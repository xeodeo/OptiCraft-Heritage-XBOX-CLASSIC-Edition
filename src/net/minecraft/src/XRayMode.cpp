#include "XRayMode.h"

#include "Minecraft.h"
#include "RenderGlobal.h"

namespace
{
bool s_active = false;
bool s_comboEnabled = false;
}

namespace XRayMode
{
bool active()
{
    return s_active;
}

bool isVisibleBlock(int blockID)
{
    switch (blockID)
    {
    case 10:  // Flowing lava
    case 11:  // Still lava
    case 14:  // Gold ore
    case 15:  // Iron ore
    case 16:  // Coal ore
    case 21:  // Lapis ore
    case 41:  // Gold block
    case 42:  // Iron block
    case 48:  // Mossy cobblestone (dungeons/altars)
    case 49:  // Obsidian
    case 52:  // Mob spawner
    case 54:  // Chest
    case 56:  // Diamond ore
    case 57:  // Diamond block
    case 73:  // Redstone ore
    case 74:  // Glowing redstone ore
    case 89:  // Glowstone
    case 120: // End portal frame
    case 129: // Emerald ore (1.5.2)
    case 146: // Trapped chest (1.5.2)
    case 153: // Nether quartz ore (1.5.2)
        return true;
    default:
        return false;
    }
}

void setActive(bool active)
{
    if (s_active == active)
        return;

    s_active = active;
    Minecraft *mc = Minecraft::getMinecraft();
    if (mc != nullptr && mc->renderGlobal != nullptr)
        mc->renderGlobal->markAllRenderersDirty();
}

void toggle()
{
    setActive(!s_active);
}

void reset()
{
    s_active = false;
}

bool comboEnabled()
{
    return s_comboEnabled;
}

void setComboEnabled(bool enabled)
{
    s_comboEnabled = enabled;
}
}
