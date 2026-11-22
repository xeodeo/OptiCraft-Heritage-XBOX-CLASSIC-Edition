#pragma once

#include "mods/IMod.h"

class ServerUtilsMod : public IMod
{
public:
    ServerUtilsMod() = default;

    std::string getId() const override { return "serverutils"; }
    std::string getName() const override { return "Server Utils"; }
    std::string getVersion() const override { return "v1.0"; }
    std::string getDescription() const override
    {
        return "Server Information & Player List buttons in the multiplayer server list.";
    }
    std::string getAuthor() const override { return "OptiCraft"; }

    bool isEnabled() const override { return m_enabled; }
    void setEnabled(bool enabled) override { m_enabled = enabled; }

    static bool isActive();

private:
    bool m_enabled = false;
};
