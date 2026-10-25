#include "EntityMinecartTNT.h"
#include "Block.h"
#include "DamageSource.h"
#include "EntityArrow.h"
#include "ItemStack.h"
#include "NBTTagCompound.h"
#include "World.h"
#include "java/Math.h"

EntityMinecartTNT::EntityMinecartTNT(World *w)
    : EntityMinecart(w), minecartTNTFuse(-1)
{
    minecartType = 3;
}

EntityMinecartTNT::EntityMinecartTNT(World *w, double x, double y, double z)
    : EntityMinecart(w, x, y, z, 3), minecartTNTFuse(-1)
{
    minecartType = 3;
}

void EntityMinecartTNT::onUpdate()
{
    EntityMinecart::onUpdate();

    if (fire > 0 && !isIgnited())
    {
        ignite();
    }

    if (minecartTNTFuse > 0)
    {
        --minecartTNTFuse;
        worldObj->spawnParticle("smoke", posX, posY + 0.5, posZ, 0.0, 0.0, 0.0);
    }
    else if (minecartTNTFuse == 0)
    {
        explodeCart(motionX * motionX + motionZ * motionZ);
    }

    if (isCollidedHorizontally)
    {
        double speedSq = motionX * motionX + motionZ * motionZ;
        if (speedSq >= 0.01)
        {
            explodeCart(speedSq);
        }
    }
}

void EntityMinecartTNT::killMinecart(const DamageSource &source)
{
    EntityMinecart::killMinecart(source);
    double speedSq = motionX * motionX + motionZ * motionZ;
    if (!source.isExplosion())
    {
        dropItemWithOffset(Block::tnt->blockID, 1, 0.0f);
    }
    if (source.isFireDamage() || source.isExplosion() || speedSq >= 0.01)
    {
        explodeCart(speedSq);
    }
}

void EntityMinecartTNT::explodeCart(double speedSq)
{
    if (!worldObj->multiplayerWorld)
    {
        double speed = JavaMath::sqrt(speedSq);
        if (speed > 5.0)
            speed = 5.0;
        worldObj->createExplosion(this, posX, posY, posZ, (float)(4.0 + rand.nextDouble() * 1.5 * speed));
        setEntityDead();
    }
}

void EntityMinecartTNT::fall(float dist)
{
    if (dist >= 3.0f)
    {
        float f = dist / 10.0f;
        explodeCart((double)(f * f));
    }
    EntityMinecart::fall(dist);
}

void EntityMinecartTNT::onActivatorRailPass(int_t x, int_t y, int_t z, bool powered)
{
    (void)x; (void)y; (void)z;
    if (powered && minecartTNTFuse < 0)
    {
        ignite();
    }
}

void EntityMinecartTNT::handleHealthUpdate(byte_t id)
{
    if (id == 10)
    {
        ignite();
    }
    else
    {
        EntityMinecart::handleHealthUpdate(id);
    }
}

void EntityMinecartTNT::ignite()
{
    minecartTNTFuse = 80;
    if (!worldObj->multiplayerWorld)
    {
        worldObj->setEntityState(this, (byte_t)10);
        worldObj->playSoundAtEntity(this, "random.fuse", 1.0f, 1.0f);
    }
}

bool EntityMinecartTNT::attackEntityFrom(const DamageSource &source, int_t damage)
{
    Entity *entity = source.getSourceOfDamage();
    EntityArrow *arrow = dynamic_cast<EntityArrow*>(entity);
    if (arrow != nullptr && arrow->isBurning())
    {
        explodeCart(arrow->motionX * arrow->motionX + arrow->motionY * arrow->motionY + arrow->motionZ * arrow->motionZ);
        return true;
    }
    if (source.isFireDamage())
    {
        if (!isIgnited())
        {
            ignite();
        }
        return true;
    }
    return EntityMinecart::attackEntityFrom(source, damage);
}

bool EntityMinecartTNT::attackEntityFrom(Entity *entity, int_t damage)
{
    EntityArrow *arrow = dynamic_cast<EntityArrow*>(entity);
    if (arrow != nullptr && arrow->isBurning())
    {
        explodeCart(arrow->motionX * arrow->motionX + arrow->motionY * arrow->motionY + arrow->motionZ * arrow->motionZ);
        return true;
    }
    return EntityMinecart::attackEntityFrom(entity, damage);
}

void EntityMinecartTNT::readEntityFromNBT(NBTTagCompound *tag)
{
    EntityMinecart::readEntityFromNBT(tag);
    if (tag->hasKey("TNTFuse"))
    {
        minecartTNTFuse = tag->getInteger("TNTFuse");
    }
}

void EntityMinecartTNT::writeEntityToNBT(NBTTagCompound *tag)
{
    EntityMinecart::writeEntityToNBT(tag);
    tag->setInteger("TNTFuse", minecartTNTFuse);
}
