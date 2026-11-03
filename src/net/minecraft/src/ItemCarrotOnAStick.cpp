#include "ItemCarrotOnAStick.h"
#include "ItemStack.h"
#include "Item.h"
#include "EntityPlayer.h"
#include "EntityPig.h"
#include "EntityAIControlledByPlayer.h"

ItemCarrotOnAStick::ItemCarrotOnAStick(int id)
    : Item(id)
{
    setMaxStackSize(1);
    setMaxDamage(25);
}

bool ItemCarrotOnAStick::isFull3D()
{
    return true;
}

bool ItemCarrotOnAStick::shouldRotateAroundWhenRendering()
{
    return true;
}

ItemStack *ItemCarrotOnAStick::onItemRightClick(ItemStack *itemstack, World *world, EntityPlayer *entityplayer)
{
    if (entityplayer != nullptr && entityplayer->isRiding() && dynamic_cast<EntityPig *>(entityplayer->ridingEntity) != nullptr)
    {
        EntityPig *pig = dynamic_cast<EntityPig *>(entityplayer->ridingEntity);
        if (pig != nullptr && pig->getAIControlledByPlayer() != nullptr)
        {
            if (pig->getAIControlledByPlayer()->isControlledByPlayer() && itemstack->getMaxDamage() - itemstack->getItemDamage() >= 7)
            {
                pig->getAIControlledByPlayer()->boostSpeed();
                itemstack->damageItem(7, entityplayer);
                if (itemstack->stackSize == 0)
                {
                    ItemStack *rod = new ItemStack(Item::fishingRod);
                    rod->setTagCompound(itemstack->getTagCompound() != nullptr ? static_cast<NBTTagCompound *>(itemstack->getTagCompound()->copy()) : nullptr);
                    return rod;
                }
            }
        }
    }
    return itemstack;
}
