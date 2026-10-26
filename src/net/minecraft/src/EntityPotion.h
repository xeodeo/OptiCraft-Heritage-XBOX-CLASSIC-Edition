#pragma once

#include "EntityThrowable.h"

class ItemStack;
class NBTTagCompound;

// net.minecraft.src.EntityPotion
class EntityPotion : public EntityThrowable
{
public:
    explicit EntityPotion(World *world);
    EntityPotion(World *world, EntityLiving *thrower, int damage);
    EntityPotion(World *world, double x, double y, double z, int damage);
    EntityPotion(World *world, EntityLiving *thrower, ItemStack *potionStack);
    EntityPotion(World *world, double x, double y, double z, ItemStack *potionStack);
    ~EntityPotion() override;

    int getPotionDamage() const;
    void setPotionDamage(int damage);

protected:
    float getGravityVelocity() const override;
    float getVelocity() const override;
    float getInaccuracyPitchOffset() const override;
    void onImpact(MovingObjectPosition *hit) override;
    void readEntityFromNBT(NBTTagCompound *tag) override;
    void writeEntityToNBT(NBTTagCompound *tag) override;

private:
    ItemStack *potionStack = nullptr;
};
