#include "EntityDragonPart.h"

#include "DamageSource.h"
#include "EntityDragonBase.h"

EntityDragonPart::EntityDragonPart(EntityDragonBase *dragon, const jstring &partName, float partWidth, float partHeight)
    : Entity(dragon->worldObj), entityDragonObj(dragon), name(partName)
{
    ensureEntityInit();
    setSize(partWidth, partHeight);
}

void EntityDragonPart::entityInit()
{
}

void EntityDragonPart::readEntityFromNBT(NBTTagCompound *)
{
}

void EntityDragonPart::writeEntityToNBT(NBTTagCompound *)
{
}

void EntityDragonPart::onUpdate()
{
    // Hitbox sub-parts are repositioned directly by EntityDragon::updateDragon()
    // every tick and are never added to loadedEntityList or driven by fluid/fire
    // physics. Skipping Entity::onEntityUpdate() avoids ~928 redundant getBlockId()
    // water/lava scans per dragon tick across the seven parts.
    ++ticksExisted;
    prevDistanceWalkedModified = distanceWalkedModified;
    prevPosX = posX;
    prevPosY = posY;
    prevPosZ = posZ;
    prevRotationPitch = rotationPitch;
    prevRotationYaw = rotationYaw;
}

bool EntityDragonPart::canBeCollidedWith()
{
    return true;
}

bool EntityDragonPart::attackEntityFrom(const DamageSource &source, int_t damage)
{
    return entityDragonObj->attackEntityFromPart(this, source, damage);
}

bool EntityDragonPart::isEntityEqual(Entity *entity)
{
    return this == entity || entityDragonObj == entity;
}
