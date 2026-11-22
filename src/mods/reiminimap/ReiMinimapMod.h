#pragma once

#include "mods/IMod.h"

class ReiMinimapMod : public IMod
{
public:
    ReiMinimapMod();
    ~ReiMinimapMod() override = default;

    std::string getId() const override { return "reiminimap"; }
    std::string getName() const override { return "Rei & Xaero Minimap"; }
    std::string getVersion() const override { return "v3.5"; }
    std::string getDescription() const override { return "Round/Square map, radar, cave mode, biome & waypoints (Back+D-Pad)."; }
    std::string getAuthor() const override { return "ReiFNSK / OptiCraft"; }

    bool isEnabled() const override;
    void setEnabled(bool state) override;

    bool hasConfigScreen() const override { return true; }
    GuiScreen *createConfigScreen(GuiScreen *parent) override;

    void onInit(Minecraft *mc) override;
    void onTick() override;
    void onRenderGameOverlay(GuiIngame *gui, int_t screenWidth, int_t screenHeight, float_t partialTick) override;

private:
    bool m_enabled;
};
