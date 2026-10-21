#include "ItemSlab.h"

#include "AxisAlignedBB.h"
#include "Block.h"
#include "BlockHalfSlab.h"
#include "BlockStep.h"
#include "EntityPlayer.h"
#include "ItemStack.h"
#include "StepSound.h"
#include "World.h"

ItemSlab::ItemSlab(int id, BlockHalfSlab* singleSlab, BlockHalfSlab* doubleSlabIn, bool isDouble)
    : ItemBlock(id)
    , theHalfSlab(singleSlab)
    , doubleSlab(doubleSlabIn)
    , isFullBlock(isDouble)
{
    setMaxDamage(0);
    setHasSubtypes(true);
}

ItemSlab::ItemSlab(int id)
    : ItemBlock(id)
    , theHalfSlab(static_cast<BlockHalfSlab*>(Block::stairSingle))
    , doubleSlab(static_cast<BlockHalfSlab*>(Block::stairDouble))
    , isFullBlock(Block::stairDouble != nullptr && (id + 256) == Block::stairDouble->blockID)
{
    setMaxDamage(0);
    setHasSubtypes(true);
}

int ItemSlab::getIconFromDamage(int damage)
{
    Block* block = Block::blocksList[shiftedIndex];
    if (block != nullptr)
    {
        return block->getBlockTextureFromSideAndMetadata(2, damage);
    }
    return 0;
}

int ItemSlab::getMetadata(int damage)
{
    return damage;
}

std::string ItemSlab::getItemNameIS(ItemStack* itemstack)
{
    if (theHalfSlab != nullptr)
    {
        return theHalfSlab->getFullSlabName(itemstack->getItemDamage());
    }
    return getItemName();
}

bool ItemSlab::onItemUse(ItemStack* itemstack, EntityPlayer* entityplayer, World* world, int i, int j, int k, int side)
{
    if (isFullBlock)
    {
        return ItemBlock::onItemUse(itemstack, entityplayer, world, i, j, k, side);
    }
    if (itemstack->stackSize == 0)
    {
        return false;
    }
    if (entityplayer != nullptr && !entityplayer->canPlayerEdit(i, j, k))
    {
        return false;
    }
    if (theHalfSlab == nullptr || doubleSlab == nullptr)
    {
        return ItemBlock::onItemUse(itemstack, entityplayer, world, i, j, k, side);
    }

    const int blockId = world->getBlockId(i, j, k);
    const int metadata = world->getBlockMetadata(i, j, k);
    const int slabType = metadata & 7;
    const bool topHalf = (metadata & 8) != 0;
    if ((side == 1 && !topHalf || side == 0 && topHalf) &&
        blockId == theHalfSlab->blockID && slabType == itemstack->getItemDamage())
    {
        AxisAlignedBB *bounds = doubleSlab->getCollisionBoundingBoxFromPool(world, i, j, k);
        const bool clear = bounds != nullptr && world->checkIfAABBIsClear(bounds);
        if (clear && world->setBlockAndMetadataWithNotify(i, j, k, doubleSlab->blockID, slabType))
        {
            world->playSoundEffect((double)((float)i + 0.5f), (double)((float)j + 0.5f), (double)((float)k + 0.5f),
                                   doubleSlab->stepSound->getStepSound(),
                                   (doubleSlab->stepSound->getVolume() + 1.0f) / 2.0f,
                                   doubleSlab->stepSound->getPitch() * 0.8f);
            --itemstack->stackSize;
        }
        return true;
    }

    if (tryPlaceDoubleSlab(itemstack, entityplayer, world, i, j, k, side))
    {
        return true;
    }
    return ItemBlock::onItemUse(itemstack, entityplayer, world, i, j, k, side);
}

bool ItemSlab::tryPlaceDoubleSlab(ItemStack* itemstack, EntityPlayer* entityplayer, World* world, int i, int j, int k, int side)
{
    (void)entityplayer;
    if (theHalfSlab == nullptr || doubleSlab == nullptr)
    {
        return false;
    }
    if (side == 0) --j;
    if (side == 1) ++j;
    if (side == 2) --k;
    if (side == 3) ++k;
    if (side == 4) --i;
    if (side == 5) ++i;

    const int blockId = world->getBlockId(i, j, k);
    const int metadata = world->getBlockMetadata(i, j, k);
    const int slabType = metadata & 7;
    if (blockId != theHalfSlab->blockID || slabType != itemstack->getItemDamage())
    {
        return false;
    }

    AxisAlignedBB *bounds = doubleSlab->getCollisionBoundingBoxFromPool(world, i, j, k);
    const bool clear = bounds != nullptr && world->checkIfAABBIsClear(bounds);
    if (clear && world->setBlockAndMetadataWithNotify(i, j, k, doubleSlab->blockID, slabType))
    {
        world->playSoundEffect((double)((float)i + 0.5f), (double)((float)j + 0.5f), (double)((float)k + 0.5f),
                               doubleSlab->stepSound->getStepSound(),
                               (doubleSlab->stepSound->getVolume() + 1.0f) / 2.0f,
                               doubleSlab->stepSound->getPitch() * 0.8f);
        --itemstack->stackSize;
        return true;
    }
    return false;
}
