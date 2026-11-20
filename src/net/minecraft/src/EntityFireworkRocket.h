#pragma once

#include "Entity.h"

class ItemStack;
class NBTTagCompound;
class World;

// net.minecraft.src.EntityFireworkRocket
class EntityFireworkRocket : public Entity
{
public:
    explicit EntityFireworkRocket(World *world);
    EntityFireworkRocket(World *world, double x, double y, double z, ItemStack *fireworkStack);

    bool isInRangeToRenderDist(double distSq) override;
    void setVelocity(double vx, double vy, double vz) override;
    void onUpdate() override;
    void handleHealthUpdate(byte_t state) override;
    void writeEntityToNBT(NBTTagCompound *nbttagcompound) override;
    void readEntityFromNBT(NBTTagCompound *nbttagcompound) override;
    float getShadowSize() override;
    bool canAttackWithItem() override;

    int_t getFireworkAge() const { return fireworkAge; }
    int_t getLifetime() const { return lifetime; }

protected:
    void entityInit() override;

private:
    int_t fireworkAge;
    int_t lifetime;
};
