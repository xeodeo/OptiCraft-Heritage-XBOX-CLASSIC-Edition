#include "EntityHanging.h"

#include <algorithm>
#include <vector>
#include "AxisAlignedBB.h"
#include "DamageSource.h"
#include "Direction.h"
#include "EntityPlayer.h"
#include "Material.h"
#include "MathHelper.h"
#include "NBTTagCompound.h"
#include "World.h"

EntityHanging::EntityHanging(World *world)
    : Entity(world)
{
    yOffset = 0.0f;
    setSize(0.5f, 0.5f);
}

EntityHanging::EntityHanging(World *world, int x, int y, int z, int dir)
    : EntityHanging(world)
{
    xPosition = x;
    yPosition = y;
    zPosition = z;
    (void)dir;
}

void EntityHanging::entityInit()
{
}

void EntityHanging::setDirection(int dir)
{
    hangingDirection = dir;
    prevRotationYaw = rotationYaw = static_cast<float>(dir * 90);
    float widthF = static_cast<float>(func_82329_d());
    float heightF = static_cast<float>(func_82330_g());
    float depthF = static_cast<float>(func_82329_d());

    if (dir != 2 && dir != 0)
    {
        widthF = 0.5f;
    }
    else
    {
        depthF = 0.5f;
        rotationYaw = prevRotationYaw = static_cast<float>(Direction::rotateOpposite[dir] * 90);
    }

    widthF /= 32.0f;
    heightF /= 32.0f;
    depthF /= 32.0f;

    float posXF = static_cast<float>(xPosition) + 0.5f;
    float posYF = static_cast<float>(yPosition) + 0.5f;
    float posZF = static_cast<float>(zPosition) + 0.5f;
    constexpr float offset = 0.5625f;

    if (dir == 2) posZF -= offset;
    if (dir == 1) posXF -= offset;
    if (dir == 0) posZF += offset;
    if (dir == 3) posXF += offset;

    if (dir == 2) posXF -= func_70517_b(func_82329_d());
    if (dir == 1) posZF += func_70517_b(func_82329_d());
    if (dir == 0) posXF += func_70517_b(func_82329_d());
    if (dir == 3) posZF -= func_70517_b(func_82329_d());

    posYF += func_70517_b(func_82330_g());
    setPosition(static_cast<double>(posXF), static_cast<double>(posYF), static_cast<double>(posZF));

    constexpr float margin = -0.03125f;
    boundingBox->setBounds(
        static_cast<double>(posXF - widthF - margin),
        static_cast<double>(posYF - heightF - margin),
        static_cast<double>(posZF - depthF - margin),
        static_cast<double>(posXF + widthF + margin),
        static_cast<double>(posYF + heightF + margin),
        static_cast<double>(posZF + depthF + margin)
    );
}

float EntityHanging::func_70517_b(int size)
{
    if (size == 32)
        return 0.5f;
    return (size == 64) ? 0.5f : 0.0f;
}

void EntityHanging::onUpdate()
{
    if (tickCounter1++ == 100 && worldObj != nullptr && !worldObj->multiplayerWorld)
    {
        tickCounter1 = 0;
        if (!isDead && !onValidSurface())
        {
            setDead();
            dropItemStack();
        }
    }
}

bool EntityHanging::onValidSurface()
{
    if (worldObj == nullptr)
        return false;

    if (!worldObj->getCollidingBoundingBoxes(this, boundingBox).empty())
        return false;

    int w = std::max(1, func_82329_d() / 16);
    int h = std::max(1, func_82330_g() / 16);
    int blockX = xPosition;
    int blockY = yPosition;
    int blockZ = zPosition;

    if (hangingDirection == 2 || hangingDirection == 0)
    {
        blockX = MathHelper::floor_double(posX - static_cast<double>(static_cast<float>(func_82329_d()) / 32.0f));
    }
    if (hangingDirection == 1 || hangingDirection == 3)
    {
        blockZ = MathHelper::floor_double(posZ - static_cast<double>(static_cast<float>(func_82329_d()) / 32.0f));
    }

    blockY = MathHelper::floor_double(posY - static_cast<double>(static_cast<float>(func_82330_g()) / 32.0f));

    for (int ix = 0; ix < w; ++ix)
    {
        for (int iy = 0; iy < h; ++iy)
        {
            Material *mat = nullptr;
            if (hangingDirection != 2 && hangingDirection != 0)
            {
                mat = worldObj->getBlockMaterial(xPosition, blockY + iy, blockZ + ix);
            }
            else
            {
                mat = worldObj->getBlockMaterial(blockX + ix, blockY + iy, zPosition);
            }

            if (mat == nullptr || !mat->isSolid())
            {
                return false;
            }
        }
    }

    std::vector<Entity *> nearby = worldObj->getEntitiesWithinAABBExcludingEntity(this, boundingBox);
    for (Entity *ent : nearby)
    {
        if (dynamic_cast<EntityHanging *>(ent) != nullptr)
        {
            return false;
        }
    }

    return true;
}

bool EntityHanging::canBeCollidedWith()
{
    return true;
}

bool EntityHanging::func_85031_j(Entity *entity)
{
    EntityPlayer *player = dynamic_cast<EntityPlayer *>(entity);
    if (player != nullptr)
    {
        return attackEntityFrom(DamageSource::causePlayerDamage(player), 0);
    }
    return false;
}

bool EntityHanging::attackEntityFrom(const DamageSource &source, int damage)
{
    (void)damage;
    if (isEntityInvulnerable())
        return false;

    if (!isDead && worldObj != nullptr && !worldObj->multiplayerWorld)
    {
        setDead();
        setBeenAttacked();
        EntityPlayer *player = dynamic_cast<EntityPlayer *>(source.getEntity());
        if (player != nullptr && player->capabilities.isCreativeMode)
        {
            return true;
        }
        dropItemStack();
    }
    return true;
}

void EntityHanging::moveEntity(double x, double y, double z)
{
    if (worldObj != nullptr && !worldObj->multiplayerWorld && !isDead && (x * x + y * y + z * z > 0.0))
    {
        setDead();
        dropItemStack();
    }
}

void EntityHanging::addVelocity(double x, double y, double z)
{
    if (worldObj != nullptr && !worldObj->multiplayerWorld && !isDead && (x * x + y * y + z * z > 0.0))
    {
        setDead();
        dropItemStack();
    }
}

void EntityHanging::writeEntityToNBT(NBTTagCompound *nbt)
{
    if (nbt == nullptr)
        return;

    nbt->setByte("Direction", static_cast<byte_t>(hangingDirection));
    nbt->setInteger("TileX", xPosition);
    nbt->setInteger("TileY", yPosition);
    nbt->setInteger("TileZ", zPosition);

    switch (hangingDirection)
    {
    case 0: nbt->setByte("Dir", 2); break;
    case 1: nbt->setByte("Dir", 1); break;
    case 2: nbt->setByte("Dir", 0); break;
    case 3: nbt->setByte("Dir", 3); break;
    }
}

void EntityHanging::readEntityFromNBT(NBTTagCompound *nbt)
{
    if (nbt == nullptr)
        return;

    if (nbt->hasKey("Direction"))
    {
        hangingDirection = nbt->getByte("Direction");
    }
    else
    {
        switch (nbt->getByte("Dir"))
        {
        case 0: hangingDirection = 2; break;
        case 1: hangingDirection = 1; break;
        case 2: hangingDirection = 0; break;
        case 3: hangingDirection = 3; break;
        }
    }

    xPosition = nbt->getInteger("TileX");
    yPosition = nbt->getInteger("TileY");
    zPosition = nbt->getInteger("TileZ");
    setDirection(hangingDirection);
}
