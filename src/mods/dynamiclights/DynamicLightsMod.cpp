#include "mods/dynamiclights/DynamicLightsMod.h"
#include "net/minecraft/src/DynamicLights.h"

void DynamicLightsMod::setEnabled(bool enabled)
{
    m_enabled = enabled;
    DynamicLights::setModEnabled(enabled);
}

void DynamicLightsMod::onInit(Minecraft *mc)
{
    (void)mc;
    DynamicLights::setModEnabled(m_enabled);
}
