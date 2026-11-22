#pragma once

#include "mods/IMod.h"

class Minecraft;
class EntityPlayerSP;

// Compiled Village Locator mod (ported from erluza/main and adapted for Xbox).
class VillageLocatorMod : public IMod
{
public:
    VillageLocatorMod();

    std::string getId() const override { return "villagelocator"; }
    std::string getName() const override { return "Village Locator"; }
    std::string getVersion() const override { return "v1.0"; }
    std::string getDescription() const override
    {
        return "Locates the nearest village in chat. Combo: Back+Y (Xbox) / F8 (PC).";
    }
    std::string getAuthor() const override { return "OptiCraft"; }

    bool isEnabled() const override { return m_enabled; }
    void setEnabled(bool enabled) override;

    void onInit(Minecraft *mc) override;
    void onTick() override;

private:
    void triggerLocator(EntityPlayerSP *player);

    Minecraft *m_mc;
    bool m_enabled;
    bool m_comboWasPressed;
};
