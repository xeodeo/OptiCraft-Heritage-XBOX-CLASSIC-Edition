#pragma once

#include "GuiContainer.h"
#include "ICrafting.h"

class ContainerRepair;
class GuiTextField;
class InventoryPlayer;
class World;
class EntityPlayer;

class GuiRepair : public GuiContainer, public ICrafting
{
public:
    GuiRepair(InventoryPlayer *playerInv, World *world, int x, int y, int z, EntityPlayer *player);
    ~GuiRepair() override;

    void initGui() override;
    void onGuiClosed() override;
    void drawScreen(int_t mouseX, int_t mouseY, float_t partialTick) override;
    void updateScreen() override;

    void sendSlotContents(Container *container, int_t slotIndex, ItemStack *itemstack) override;
    void sendProgressBarUpdate(Container *container, int_t varToUpdate, int_t newValue) override;

protected:
    void drawGuiContainerForegroundLayer() override;
    void drawGuiContainerBackgroundLayer(float_t partialTick) override;
    void keyTyped(char_t c, int_t key) override;
    void mouseClicked(int_t mouseX, int_t mouseY, int_t button) override;

private:
    ContainerRepair *repairContainer = nullptr;
    GuiTextField *itemNameField = nullptr;
    InventoryPlayer *field_82325_q = nullptr;
};
