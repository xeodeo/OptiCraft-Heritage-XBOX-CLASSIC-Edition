#include "CreativeCrafting.h"

#include "Minecraft.h"
#include "PlayerController.h"

CreativeCrafting::CreativeCrafting(Minecraft *minecraft)
    : mc(minecraft)
{
}

void CreativeCrafting::sendSlotContents(Container *container, int_t slotIndex, ItemStack *itemstack)
{
    (void)container;
    if (mc != nullptr && mc->playerController != nullptr)
    {
        mc->playerController->sendSlotPacket(itemstack, slotIndex);
    }
}

void CreativeCrafting::sendProgressBarUpdate(Container *container, int_t varToUpdate, int_t newValue)
{
    (void)container;
    (void)varToUpdate;
    (void)newValue;
}
