#pragma once
#include "EntityPainting.h"

class EntityItemFrame : public EntityPainting {
public:
    explicit EntityItemFrame(World *world);
    EntityItemFrame(World *world, int_t x, int_t y, int_t z, int_t dir);
    ~EntityItemFrame() override;
protected:
    void entityInit() override;
public:
    ItemStack *getDisplayedItem() const;
    void setDisplayedItem(ItemStack *stack);
    int_t getRotation() const;
    void setItemRotation(int_t rotation);
    bool interact(EntityPlayer *player) override;
    void writeEntityToNBT(NBTTagCompound *tag) override;
    void readEntityFromNBT(NBTTagCompound *tag) override;
    bool attackEntityFrom(Entity *entity, int_t damage) override;
private:
    void dropItemStack();
    float itemDropChance = 1.0f;
};
