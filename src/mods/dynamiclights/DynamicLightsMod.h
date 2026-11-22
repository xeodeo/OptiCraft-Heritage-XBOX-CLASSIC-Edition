#pragma once

#include "mods/IMod.h"

class DynamicLightsMod : public IMod
{
public:
    DynamicLightsMod() = default;

    std::string getId() const override { return "dynamiclights"; }
    std::string getName() const override { return "Dynamic Lights"; }
    std::string getVersion() const override { return "v1.0"; }
    std::string getDescription() const override
    {
        return "Held torches, glowstone & burning entities emit real-time light.";
    }
    std::string getAuthor() const override { return "OptiCraft"; }

    bool isEnabled() const override { return m_enabled; }
    void setEnabled(bool enabled) override;
    void onInit(Minecraft *mc) override;

private:
    bool m_enabled = false;
};
