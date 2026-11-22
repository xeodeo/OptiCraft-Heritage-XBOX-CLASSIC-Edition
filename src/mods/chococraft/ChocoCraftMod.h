#pragma once

#include "mods/IMod.h"
#include "java/Type.h"

class World;
class EntityPlayer;

class ChocoCraftMod : public IMod
{
public:
    ChocoCraftMod();
    ~ChocoCraftMod() override = default;

    static ChocoCraftMod *getInstance();

    std::string getId() const override { return "chococraft"; }
    std::string getName() const override { return "ChocoCraft"; }
    std::string getVersion() const override { return "v1.0"; }
    std::string getDescription() const override { return "Wild chocobos, Gysahl greens, breeding colors, saddlebags & flight."; }
    std::string getAuthor() const override { return "OptiCraft"; }

    bool isEnabled() const override { return m_enabled; }
    void setEnabled(bool state) override { m_enabled = state; }

    void onInit(Minecraft *mc) override;
    void onTick() override;

private:
    void trySpawnWildChocobo(World *world, EntityPlayer *player);
    void trySpawnWildGysahl(World *world, EntityPlayer *player);

    Minecraft *m_mc;
    bool m_enabled;
    int_t m_spawnTimer;
    int_t m_plantTimer;
};
