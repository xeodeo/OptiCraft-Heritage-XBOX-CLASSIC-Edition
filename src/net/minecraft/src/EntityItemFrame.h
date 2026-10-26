#pragma once

#include "EntityHanging.h"

class World;
class ItemStack;
class NBTTagCompound;
class EntityPlayer;

class EntityItemFrame : public EntityHanging
{
public:
    explicit EntityItemFrame(World *world);
    EntityItemFrame(World *world, int x, int y, int z, int dir);
    ~EntityItemFrame() override = default;

    int func_82329_d() override;
    int func_82330_g() override;
    bool isInRangeToRenderDist(double dist) override;
    void dropItemStack() override;

    ItemStack *getDisplayedItem();
    void setDisplayedItem(ItemStack *stack);
    int getRotation();
    void setItemRotation(int rot);

    void writeEntityToNBT(NBTTagCompound *nbt) override;
    void readEntityFromNBT(NBTTagCompound *nbt) override;
    bool interact(EntityPlayer *player) override;

protected:
    void entityInit() override;

private:
    float itemDropChance = 1.0f;
};
