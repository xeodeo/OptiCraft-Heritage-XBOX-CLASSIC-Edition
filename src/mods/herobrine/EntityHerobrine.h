#pragma once

#include "net/minecraft/src/EntityCreature.h"
#include "net/minecraft/src/ItemStack.h"

class World;
class NBTTagCompound;
class DamageSource;

class EntityHerobrine : public EntityCreature
{
public:
    enum class EventMode : int
    {
        Ghost = 0,     // iLoveYou GhostEvent: holds redstone torch, stares, vanishes if player approaches
        Behind = 1,    // Appears behind player, vanishes on turn/stare
        Altar = 2,     // iLoveYou AltarEvent: stands atop altar with redstone torch until ritual ends
        Digger = 3,    // iLoveYou HeroDigger: holds diamond shovel, retreats digging tunnel to underground room
        Diamond = 4,   // iLoveYou DiamondEvent: holds diamond pickaxe, digs shaft to diamond ore
        Thief = 5      // iLoveYou ThiefEvent: stands atop chest with redstone torch, vanishes when player <10 blocks
    };

    enum class HeldTool : int
    {
        None = 0,
        RedstoneTorch = 1,
        DiamondShovel = 2,
        DiamondPickaxe = 3
    };

    explicit EntityHerobrine(World *world);
    ~EntityHerobrine() override;

    void setEventMode(EventMode mode) { m_eventMode = mode; }
    EventMode getEventMode() const { return m_eventMode; }

    void setHeldTool(HeldTool tool);
    HeldTool getHeldTool() const { return m_heldTool; }
    ItemStack *getHeldItem() override { return m_heldStack; }

    int_t getMaxHealth() const override { return 40; }
    bool canDespawn() override { return false; }
    void onLivingUpdate() override;
    bool attackEntityFrom(Entity *entity, int_t damage) override;
    bool attackEntityFrom(const DamageSource &source, int_t damage) override;
    void writeEntityToNBT(NBTTagCompound *nbt) override;
    void readEntityFromNBT(NBTTagCompound *nbt) override;

    void vanishWithEffects();

protected:
    jstring getLivingSound() override { return ""; }
    jstring getHurtSound() override { return ""; }
    jstring getDeathSound() override { return ""; }

private:
    EventMode m_eventMode;
    HeldTool m_heldTool;
    ItemStack *m_heldStack;
    int_t m_lifeTicks;
    int_t m_stareTicks;
    bool m_spotted;
    double m_lastPlayerDist;
};
