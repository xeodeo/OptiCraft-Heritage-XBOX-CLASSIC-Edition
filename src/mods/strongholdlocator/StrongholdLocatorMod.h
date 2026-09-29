#pragma once

#include "mods/IMod.h"

class StrongholdLocatorMod : public IMod
{
public:
    StrongholdLocatorMod();
    ~StrongholdLocatorMod() override = default;

    std::string getId() const override { return "strongholdlocator"; }
    std::string getName() const override { return "Stronghold Locator"; }
    std::string getVersion() const override { return "v1.0"; }
    std::string getDescription() const override { return "Locates the nearest Stronghold via L3+R3 (Debug)"; }
    std::string getAuthor() const override { return "OptiCraft"; }

    bool isEnabled() const override { return m_enabled; }
    void setEnabled(bool state) override { m_enabled = state; }

    void onInit(Minecraft *mc) override;
    void onTick() override;

private:
    void triggerLocator();

    Minecraft *m_mc;
    bool m_enabled;
    bool m_comboWasPressed;
};
