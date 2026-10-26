#include "SlotRepair.h"

#include "Block.h"
#include "ContainerRepair.h"
#include "EntityPlayer.h"
#include "ItemStack.h"
#include "World.h"
#include "java/Random.h"

SlotRepair::SlotRepair(ContainerRepair *container, IInventory *inv, int index, int x, int y, World *world, int blockX, int blockY, int blockZ)
    : Slot(inv, index, x, y),
      anvil(container),
      theWorld(world),
      blockPosX(blockX),
      blockPosY(blockY),
      blockPosZ(blockZ)
{
}

bool SlotRepair::isItemValid(ItemStack *stack)
{
    (void)stack;
    return false;
}

bool SlotRepair::canTakeStack(EntityPlayer *player)
{
    if (player == nullptr || anvil == nullptr)
        return false;

    return (player->capabilities.isCreativeMode || player->experienceLevel >= anvil->maximumCost) &&
           anvil->maximumCost > 0 &&
           getHasStack();
}

void SlotRepair::onPickupFromSlot(EntityPlayer *player, ItemStack *stack)
{
    (void)stack;
    if (player == nullptr || anvil == nullptr)
        return;

    if (!player->capabilities.isCreativeMode)
    {
        player->addExperienceLevel(-anvil->maximumCost);
    }

    IInventory *input = anvil->getInputSlots();
    if (input != nullptr)
    {
        input->setInventorySlotContents(0, nullptr);
        const int usedCount = anvil->getStackSizeToBeUsedInRepair();
        if (usedCount > 0)
        {
            ItemStack *right = input->getStackInSlot(1);
            if (right != nullptr && right->stackSize > usedCount)
            {
                right->stackSize -= usedCount;
                input->setInventorySlotContents(1, right);
            }
            else
            {
                input->setInventorySlotContents(1, nullptr);
            }
        }
        else
        {
            input->setInventorySlotContents(1, nullptr);
        }
    }

    anvil->maximumCost = 0;

    if (theWorld != nullptr && !theWorld->multiplayerWorld)
    {
        if (Block::anvil != nullptr && theWorld->getBlockId(blockPosX, blockPosY, blockPosZ) == Block::anvil->blockID)
        {
            if (!player->capabilities.isCreativeMode && theWorld->rand.nextFloat() < 0.12f)
            {
                int meta = theWorld->getBlockMetadata(blockPosX, blockPosY, blockPosZ);
                int dir = meta & 3;
                int damage = meta >> 2;
                ++damage;
                if (damage > 2)
                {
                    theWorld->setBlock(blockPosX, blockPosY, blockPosZ, 0, 0, 3);
                    theWorld->playAuxSFX(1020, blockPosX, blockPosY, blockPosZ, 0);
                }
                else
                {
                    theWorld->setBlockMetadataWithNotify(blockPosX, blockPosY, blockPosZ, dir | (damage << 2), 2);
                    theWorld->playAuxSFX(1021, blockPosX, blockPosY, blockPosZ, 0);
                }
            }
            else
            {
                theWorld->playAuxSFX(1021, blockPosX, blockPosY, blockPosZ, 0);
            }
        }
    }
}
