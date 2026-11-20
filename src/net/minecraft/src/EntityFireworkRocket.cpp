#include "EntityFireworkRocket.h"

#include <cmath>

#include "DataWatcher.h"
#include "ItemStack.h"
#include "MathHelper.h"
#include "NBTTagCompound.h"
#include "World.h"

EntityFireworkRocket::EntityFireworkRocket(World *world)
    : Entity(world), fireworkAge(0), lifetime(0)
{
    ensureEntityInit();
    setSize(0.25f, 0.25f);
}

EntityFireworkRocket::EntityFireworkRocket(World *world, double x, double y, double z, ItemStack *fireworkStack)
    : Entity(world), fireworkAge(0), lifetime(0)
{
    ensureEntityInit();
    setSize(0.25f, 0.25f);
    setPosition(x, y, z);
    yOffset = 0.0f;

    int_t flight = 1;
    if (fireworkStack != nullptr && fireworkStack->hasTagCompound())
    {
        ItemStack *stored = fireworkStack->copy();
        stored->stackSize = 1;
        dataWatcher->updateObject(8, stored);

        NBTTagCompound *root = fireworkStack->getTagCompound();
        if (root != nullptr && root->hasKey("Fireworks"))
        {
            NBTTagCompound *fireworks = root->getCompoundTag("Fireworks");
            if (fireworks != nullptr)
            {
                flight += static_cast<int_t>(fireworks->getByte("Flight"));
            }
        }
    }

    motionX = rand.nextGaussian() * 0.001;
    motionZ = rand.nextGaussian() * 0.001;
    motionY = 0.05;
    lifetime = 10 * flight + rand.nextInt(6) + rand.nextInt(7);
}

void EntityFireworkRocket::entityInit()
{
    dataWatcher->addObjectByDataType(8, 5);
}

bool EntityFireworkRocket::isInRangeToRenderDist(double distSq)
{
    return distSq < 4096.0;
}

void EntityFireworkRocket::setVelocity(double vx, double vy, double vz)
{
    motionX = vx;
    motionY = vy;
    motionZ = vz;
    if (prevRotationPitch == 0.0f && prevRotationYaw == 0.0f)
    {
        const float horiz = MathHelper::sqrt_double(vx * vx + vz * vz);
        prevRotationYaw = rotationYaw = static_cast<float>(std::atan2(vx, vz) * 180.0 / 3.141592653589793);
        prevRotationPitch = rotationPitch = static_cast<float>(std::atan2(vy, static_cast<double>(horiz)) * 180.0 / 3.141592653589793);
    }
}

void EntityFireworkRocket::onUpdate()
{
    lastTickPosX = posX;
    lastTickPosY = posY;
    lastTickPosZ = posZ;
    Entity::onUpdate();

    motionX *= 1.15;
    motionZ *= 1.15;
    motionY += 0.04;
    moveEntity(motionX, motionY, motionZ);

    const float horiz = MathHelper::sqrt_double(motionX * motionX + motionZ * motionZ);
    rotationYaw = static_cast<float>(std::atan2(motionX, motionZ) * 180.0 / 3.141592653589793);
    rotationPitch = static_cast<float>(std::atan2(motionY, static_cast<double>(horiz)) * 180.0 / 3.141592653589793);

    int_t guard = 0;
    while (rotationPitch - prevRotationPitch < -180.0f && ++guard < 8)
        prevRotationPitch -= 360.0f;
    guard = 0;
    while (rotationPitch - prevRotationPitch >= 180.0f && ++guard < 8)
        prevRotationPitch += 360.0f;
    guard = 0;
    while (rotationYaw - prevRotationYaw < -180.0f && ++guard < 8)
        prevRotationYaw -= 360.0f;
    guard = 0;
    while (rotationYaw - prevRotationYaw >= 180.0f && ++guard < 8)
        prevRotationYaw += 360.0f;

    rotationPitch = prevRotationPitch + (rotationPitch - prevRotationPitch) * 0.2f;
    rotationYaw = prevRotationYaw + (rotationYaw - prevRotationYaw) * 0.2f;

    if (fireworkAge == 0 && worldObj != nullptr)
    {
        worldObj->playSoundAtEntity(this, "fireworks.launch", 3.0f, 1.0f);
    }

    ++fireworkAge;
    if (worldObj != nullptr && (fireworkAge % 2) < 2)
    {
        worldObj->spawnParticle("fireworksSpark",
                                posX, posY - 0.3, posZ,
                                rand.nextGaussian() * 0.05,
                                -motionY * 0.5,
                                rand.nextGaussian() * 0.05);
    }

    if (worldObj != nullptr && !worldObj->multiplayerWorld && fireworkAge > lifetime)
    {
        worldObj->setEntityState(this, 17);
        setEntityDead();
    }
}

void EntityFireworkRocket::handleHealthUpdate(byte_t state)
{
    if (state == 17 && worldObj != nullptr)
    {
        ItemStack *stack = dataWatcher->getWatchableObjectItemStack(8);
        NBTTagCompound *fireworksTag = nullptr;
        if (stack != nullptr && stack->hasTagCompound() && stack->getTagCompound()->hasKey("Fireworks"))
        {
            fireworksTag = stack->getTagCompound()->getCompoundTag("Fireworks");
        }
        worldObj->makeFireworks(posX, posY, posZ, motionX, motionY, motionZ, fireworksTag);
    }
    Entity::handleHealthUpdate(state);
}

void EntityFireworkRocket::writeEntityToNBT(NBTTagCompound *nbttagcompound)
{
    nbttagcompound->setInteger("Life", fireworkAge);
    nbttagcompound->setInteger("LifeTime", lifetime);
    ItemStack *stack = dataWatcher->getWatchableObjectItemStack(8);
    if (stack != nullptr)
    {
        NBTTagCompound *itemTag = new NBTTagCompound();
        stack->writeToNBT(itemTag);
        nbttagcompound->setCompoundTag("FireworksItem", itemTag);
    }
}

void EntityFireworkRocket::readEntityFromNBT(NBTTagCompound *nbttagcompound)
{
    fireworkAge = nbttagcompound->getInteger("Life");
    lifetime = nbttagcompound->getInteger("LifeTime");
    if (nbttagcompound->hasKey("FireworksItem"))
    {
        NBTTagCompound *itemTag = nbttagcompound->getCompoundTag("FireworksItem");
        if (itemTag != nullptr)
        {
            ItemStack *loaded = ItemStack::loadItemStackFromNBT(itemTag);
            if (loaded != nullptr)
            {
                dataWatcher->updateObject(8, loaded);
            }
        }
    }
}

float EntityFireworkRocket::getShadowSize()
{
    return 0.0f;
}

bool EntityFireworkRocket::canAttackWithItem()
{
    return false;
}
