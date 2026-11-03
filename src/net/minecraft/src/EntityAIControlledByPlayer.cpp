#include "EntityAIControlledByPlayer.h"
#include "NBTTagCompound.h"
#include "EntityLiving.h"
#include "EntityPlayer.h"
#include "EntityJumpHelper.h"
#include "World.h"
#include "Block.h"
#include "BlockHalfSlab.h"
#include "PathFinder.h"
#include "PathPoint.h"
#include "Item.h"
#include "ItemStack.h"
#include "InventoryPlayer.h"
#include "MathHelper.h"
#include <cmath>
#include <algorithm>

EntityAIControlledByPlayer::EntityAIControlledByPlayer(EntityLiving *entity, float speed)
    : thisEntity(entity)
    , maxSpeed(speed)
    , currentSpeed(0.0f)
    , speedBoosted(false)
    , speedBoostTime(0)
    , maxSpeedBoostTime(0)
{
    setMutexBits(7);
}

void EntityAIControlledByPlayer::startExecuting()
{
    currentSpeed = 0.0f;
}

void EntityAIControlledByPlayer::resetTask()
{
    speedBoosted = false;
    currentSpeed = 0.0f;
}

bool EntityAIControlledByPlayer::shouldExecute()
{
    return thisEntity != nullptr
        && thisEntity->isEntityAlive()
        && thisEntity->riddenByEntity != nullptr
        && dynamic_cast<EntityPlayer *>(thisEntity->riddenByEntity) != nullptr
        && (speedBoosted || thisEntity->canBeSteered());
}

void EntityAIControlledByPlayer::updateTask()
{
    if (thisEntity == nullptr || thisEntity->riddenByEntity == nullptr)
    {
        return;
    }

    EntityPlayer *player = dynamic_cast<EntityPlayer *>(thisEntity->riddenByEntity);
    if (player == nullptr)
    {
        return;
    }

    float diffYaw = MathHelper::wrapAngleTo180_float(player->rotationYaw - thisEntity->rotationYaw) * 0.5f;
    if (diffYaw > 5.0f)
    {
        diffYaw = 5.0f;
    }
    if (diffYaw < -5.0f)
    {
        diffYaw = -5.0f;
    }

    thisEntity->rotationYaw = MathHelper::wrapAngleTo180_float(thisEntity->rotationYaw + diffYaw);
    if (currentSpeed < maxSpeed)
    {
        currentSpeed += (maxSpeed - currentSpeed) * 0.01f;
    }
    if (currentSpeed > maxSpeed)
    {
        currentSpeed = maxSpeed;
    }

    int blockX = MathHelper::floor_double(thisEntity->posX);
    int blockY = MathHelper::floor_double(thisEntity->posY);
    int blockZ = MathHelper::floor_double(thisEntity->posZ);

    float speed = currentSpeed;
    if (speedBoosted)
    {
        if (speedBoostTime++ > maxSpeedBoostTime)
        {
            speedBoosted = false;
        }
        speed += speed * 1.15f * MathHelper::sin((float)speedBoostTime / (float)maxSpeedBoostTime * (float)3.141592653589793);
    }

    float slipperiness = 0.91f;
    if (thisEntity->onGround)
    {
        slipperiness = 0.54600006f;
        int belowBlock = thisEntity->worldObj->getBlockId(blockX, blockY - 1, blockZ);
        if (belowBlock > 0 && Block::blocksList[belowBlock] != nullptr)
        {
            slipperiness = Block::blocksList[belowBlock]->slipperiness * 0.91f;
        }
    }

    float factor = 0.16277136f / (slipperiness * slipperiness * slipperiness);
    float sinYaw = MathHelper::sin(thisEntity->rotationYaw * (float)3.141592653589793 / 180.0f);
    float cosYaw = MathHelper::cos(thisEntity->rotationYaw * (float)3.141592653589793 / 180.0f);
    float scaledSpeed = thisEntity->getAIMoveSpeed() * factor;
    float maxS = std::max(speed, 1.0f);
    maxS = scaledSpeed / maxS;
    float moveS = speed * maxS;
    float moveX = -(moveS * sinYaw);
    float moveZ = moveS * cosYaw;

    if (std::abs(moveX) > std::abs(moveZ))
    {
        if (moveX < 0.0f)
        {
            moveX -= thisEntity->width / 2.0f;
        }
        if (moveX > 0.0f)
        {
            moveX += thisEntity->width / 2.0f;
        }
        moveZ = 0.0f;
    }
    else
    {
        moveX = 0.0f;
        if (moveZ < 0.0f)
        {
            moveZ -= thisEntity->width / 2.0f;
        }
        if (moveZ > 0.0f)
        {
            moveZ += thisEntity->width / 2.0f;
        }
    }

    int nextX = MathHelper::floor_double(thisEntity->posX + (double)moveX);
    int nextZ = MathHelper::floor_double(thisEntity->posZ + (double)moveZ);
    PathPoint sizePoint(
        MathHelper::floor_float(thisEntity->width + 1.0f),
        MathHelper::floor_float(thisEntity->height + player->height + 1.0f),
        MathHelper::floor_float(thisEntity->width + 1.0f)
    );

    if (blockX != nextX || blockZ != nextZ)
    {
        int curBlock = thisEntity->worldObj->getBlockId(blockX, blockY, blockZ);
        int curBelowBlock = thisEntity->worldObj->getBlockId(blockX, blockY - 1, blockZ);
        bool isSlab = func_98216_b(curBlock) || (Block::blocksList[curBlock] == nullptr && func_98216_b(curBelowBlock));
        if (!isSlab
            && Pathfinder::func_82565_a(thisEntity, nextX, blockY, nextZ, &sizePoint, false, false, true) == 0
            && Pathfinder::func_82565_a(thisEntity, blockX, blockY + 1, blockZ, &sizePoint, false, false, true) == 1
            && Pathfinder::func_82565_a(thisEntity, nextX, blockY + 1, nextZ, &sizePoint, false, false, true) == 1)
        {
            if (thisEntity->getJumpHelper() != nullptr)
            {
                thisEntity->getJumpHelper()->setJumping();
            }
        }
    }

    if (!player->capabilities.isCreativeMode && currentSpeed >= maxSpeed * 0.5f && thisEntity->getRNG().nextFloat() < 0.006f && !speedBoosted)
    {
        ItemStack *held = player->getHeldItem();
        if (held != nullptr && Item::carrotOnAStick != nullptr && held->itemID == Item::carrotOnAStick->shiftedIndex)
        {
            held->damageItem(1, player);
            if (held->stackSize == 0)
            {
                ItemStack *rod = new ItemStack(Item::fishingRod);
                rod->setTagCompound(held->getTagCompound() != nullptr ? static_cast<NBTTagCompound *>(held->getTagCompound()->copy()) : nullptr);
                player->inventory->mainInventory[player->inventory->currentItem] = rod;
                delete held;
            }
        }
    }

    thisEntity->moveEntityWithHeading(0.0f, speed);
}

bool EntityAIControlledByPlayer::func_98216_b(int_t blockId)
{
    return blockId > 0 && Block::blocksList[blockId] != nullptr &&
        (Block::blocksList[blockId]->getRenderType() == 10 || dynamic_cast<BlockHalfSlab *>(Block::blocksList[blockId]) != nullptr);
}

bool EntityAIControlledByPlayer::isSpeedBoosted() const
{
    return speedBoosted;
}

void EntityAIControlledByPlayer::boostSpeed()
{
    speedBoosted = true;
    speedBoostTime = 0;
    maxSpeedBoostTime = thisEntity->getRNG().nextInt(841) + 140;
}

bool EntityAIControlledByPlayer::isControlledByPlayer() const
{
    return !isSpeedBoosted() && currentSpeed > maxSpeed * 0.3f;
}
