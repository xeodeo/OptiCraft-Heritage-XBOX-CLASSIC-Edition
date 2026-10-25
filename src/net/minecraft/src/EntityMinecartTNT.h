#pragma once

#include "EntityMinecart.h"

class DamageSource;
class NBTTagCompound;
class World;

class EntityMinecartTNT : public EntityMinecart
{
public:
    EntityMinecartTNT(World *w);
    EntityMinecartTNT(World *w, double x, double y, double z);

    int_t getMinecartType() const override { return 3; }
    void onUpdate() override;
    void killMinecart(const DamageSource &source) override;
    void fall(float dist) override;
    void onActivatorRailPass(int_t x, int_t y, int_t z, bool powered) override;
    void handleHealthUpdate(byte_t id) override;
    bool attackEntityFrom(const DamageSource &source, int_t damage) override;
    bool attackEntityFrom(Entity *entity, int_t damage) override;

    void ignite();
    int_t func_94104_d() const { return minecartTNTFuse; }
    int_t getFuseTicks() const { return minecartTNTFuse; }
    bool isIgnited() const { return minecartTNTFuse > -1; }

    void readEntityFromNBT(NBTTagCompound *tag) override;
    void writeEntityToNBT(NBTTagCompound *tag) override;

protected:
    void explodeCart(double speedSq);

private:
    int_t minecartTNTFuse;
};
