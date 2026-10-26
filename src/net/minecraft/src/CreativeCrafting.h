#pragma once

#include "ICrafting.h"

class Minecraft;

// net.minecraft.src.CreativeCrafting
class CreativeCrafting : public ICrafting
{
public:
    explicit CreativeCrafting(Minecraft *mc);

    void sendSlotContents(Container *container, int_t slotIndex, ItemStack *itemstack) override;
    void sendProgressBarUpdate(Container *container, int_t varToUpdate, int_t newValue) override;

private:
    Minecraft *mc;
};
