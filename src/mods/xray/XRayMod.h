#pragma once

#include "mods/IMod.h"

class Minecraft;
class World;

// Compiled X-Ray mod (ported from erluza/main and adapted for Xbox).
class XRayMod : public IMod
{
public:
    XRayMod();

    std::string getId() const override { return "xray"; }
    std::string getName() const override { return "X-Ray"; }
    std::string getVersion() const override { return "v1.0"; }
    std::string getDescription() const override
    {
        return "See ores & dungeons through terrain. Combo: LT+RT+White+Black (Xbox) / F6.";
    }
    std::string getAuthor() const override { return "OptiCraft"; }

    bool isEnabled() const override { return m_enabled; }
    void setEnabled(bool enabled) override;

    void onInit(Minecraft *mc) override;
    void onTick() override;

private:
    Minecraft *m_mc;
    World *m_lastWorld;
    bool m_enabled;
    bool m_comboWasPressed;
};
