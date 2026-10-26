#pragma once

#include "Entity.h"

class World;
class NBTTagCompound;
class DamageSource;

class EntityHanging : public Entity
{
public:
    explicit EntityHanging(World *world);
    EntityHanging(World *world, int x, int y, int z, int dir);
    ~EntityHanging() override = default;

    void setDirection(int dir);
    void onUpdate() override;
    bool onValidSurface();
    bool canBeCollidedWith() override;
    bool func_85031_j(Entity *entity) override;
    bool attackEntityFrom(const DamageSource &source, int damage) override;
    void moveEntity(double x, double y, double z) override;
    void addVelocity(double x, double y, double z) override;
    void writeEntityToNBT(NBTTagCompound *nbt) override;
    void readEntityFromNBT(NBTTagCompound *nbt) override;

    virtual int func_82329_d() = 0;
    virtual int func_82330_g() = 0;
    virtual void dropItemStack() = 0;

protected:
    void entityInit() override;

public:
    int hangingDirection = 0;
    int xPosition = 0;
    int yPosition = 0;
    int zPosition = 0;

private:
    float func_70517_b(int size);

    int tickCounter1 = 0;
};
