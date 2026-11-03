#include "GuiRepair.h"
#include "platform/Log.h"
#include <cstdio>

#include <string>
#include "ContainerRepair.h"
#include "EntityPlayer.h"
#include "FontRenderer.h"
#include "GuiTextField.h"
#include "InventoryPlayer.h"
#include "ItemStack.h"
#include "Minecraft.h"
#include "RenderEngine.h"
#include "Slot.h"
#include "StatCollector.h"
#include "platform/RenderAPI.h"
#include "RenderHelper.h"
#include "EntityPlayerSP.h"

GuiRepair::GuiRepair(InventoryPlayer *playerInv, World *world, int x, int y, int z, EntityPlayer *player)
    : GuiContainer(new ContainerRepair(playerInv, world, x, y, z, player), true, player),
      field_82325_q(playerInv)
{
    repairContainer = dynamic_cast<ContainerRepair *>(inventorySlots);
}

GuiRepair::~GuiRepair()
{
    // The on-screen keyboard keeps a pointer to the focused field.
    if (itemNameField != nullptr)
        itemNameField->setFocused(false);
    delete itemNameField;
    itemNameField = nullptr;
}

void GuiRepair::initGui()
{
    GuiContainer::initGui();

    int guiX = (width - xSize) / 2;
    int guiY = (height - ySize) / 2;

    if (itemNameField != nullptr)
        itemNameField->setFocused(false);
    delete itemNameField;
    itemNameField = new GuiTextField(fontRenderer, guiX + 62, guiY + 24, 103, 12);
    itemNameField->setTextColor(-1);
    itemNameField->setDisabledTextColour(-1);
    itemNameField->setEnableBackgroundDrawing(false);
    itemNameField->setMaxStringLength(30);

    if (inventorySlots != nullptr)
    {
        inventorySlots->removeCraftingFromCrafters(this);
        inventorySlots->addCraftingToCrafters(this);
    }
}

void GuiRepair::onGuiClosed()
{
    GuiContainer::onGuiClosed();
    if (inventorySlots != nullptr)
    {
        inventorySlots->removeCraftingFromCrafters(this);
    }
}

void GuiRepair::updateScreen()
{
    GuiContainer::updateScreen();
    if (itemNameField != nullptr)
    {
        itemNameField->updateCursorCounter();
    }
}

void GuiRepair::drawGuiContainerForegroundLayer()
{
    RenderHelper::disableStandardItemLighting();
    fontRenderer->drawString(StatCollector::translateToLocal("container.repair"), 60, 6, 0x404040);

    if (repairContainer != nullptr && repairContainer->maximumCost > 0)
    {
        int color = 0x80FF20;
        bool hasResult = true;
        std::string costText = StatCollector::translateToLocalFormatted("container.repair.cost", std::to_string(repairContainer->maximumCost));

        if (repairContainer->maximumCost >= 40 && (mc->thePlayer == nullptr || !mc->thePlayer->capabilities.isCreativeMode))
        {
            costText = StatCollector::translateToLocal("container.repair.expensive");
            color = 0xFF6060;
        }
        else if (repairContainer->getSlot(2) == nullptr || !repairContainer->getSlot(2)->getHasStack())
        {
            hasResult = false;
        }
        else if (field_82325_q != nullptr && !repairContainer->getSlot(2)->canTakeStack(field_82325_q->player))
        {
            color = 0xFF6060;
        }

        // TEMP diagnostic (anvil cost text, 03-11): log the exact bytes once per change.
        {
            static std::string lastLogged;
            if (costText != lastLogged)
            {
                lastLogged = costText;
                std::string hex;
                char b[4];
                for (unsigned char ch : costText) { snprintf(b, sizeof b, "%02x", ch); hex += b; }
                MC_LOG_INFO("diag.anvil", "cost=%d text=[%s] hex=%s\n", repairContainer->maximumCost, costText.c_str(), hex.c_str());
            }
        }
        if (hasResult)
        {
            int shadowColor = 0xFF000000 | ((color & 0xFCFCFC) >> 2);
            int textX = xSize - 8 - fontRenderer->getStringWidth(costText);
            int textY = 67;
            fontRenderer->drawString(costText, textX, textY + 1, shadowColor);
            fontRenderer->drawString(costText, textX + 1, textY, shadowColor);
            fontRenderer->drawString(costText, textX + 1, textY + 1, shadowColor);
            fontRenderer->drawString(costText, textX, textY, color);
        }
    }

    RenderHelper::enableGUIStandardItemLighting();
}

void GuiRepair::drawGuiContainerBackgroundLayer(float_t)
{
    renderColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    mc->renderEngine->bindTexture(mc->renderEngine->getTexture("/gui/repair.png"));

    int guiX = (width - xSize) / 2;
    int guiY = (height - ySize) / 2;
    drawTexturedModalRect(guiX, guiY, 0, 0, xSize, ySize);

    bool hasSlot0 = repairContainer != nullptr && repairContainer->getSlot(0) != nullptr && repairContainer->getSlot(0)->getHasStack();
    drawTexturedModalRect(guiX + 59, guiY + 20, 0, ySize + (hasSlot0 ? 0 : 16), 110, 16);

    bool hasSlot1 = repairContainer != nullptr && repairContainer->getSlot(1) != nullptr && repairContainer->getSlot(1)->getHasStack();
    bool hasSlot2 = repairContainer != nullptr && repairContainer->getSlot(2) != nullptr && repairContainer->getSlot(2)->getHasStack();
    if ((hasSlot0 || hasSlot1) && !hasSlot2)
    {
        drawTexturedModalRect(guiX + 99, guiY + 45, xSize, 0, 28, 21);
    }
}

void GuiRepair::keyTyped(char_t c, int_t key)
{
    if (itemNameField != nullptr && itemNameField->textboxKeyTyped(c, key))
    {
        if (repairContainer != nullptr)
        {
            repairContainer->updateItemName(itemNameField->getText());
        }
    }
    else
    {
        GuiContainer::keyTyped(c, key);
    }
}

void GuiRepair::mouseClicked(int_t mouseX, int_t mouseY, int_t button)
{
    GuiContainer::mouseClicked(mouseX, mouseY, button);
    if (itemNameField != nullptr)
    {
        itemNameField->mouseClicked(mouseX, mouseY, button);
    }
}

void GuiRepair::drawScreen(int_t mouseX, int_t mouseY, float_t partialTick)
{
    GuiContainer::drawScreen(mouseX, mouseY, partialTick);
    RenderHelper::disableStandardItemLighting();
    if (itemNameField != nullptr)
    {
        itemNameField->drawTextBox();
    }
}

void GuiRepair::sendSlotContents(Container *, int_t slotIndex, ItemStack *itemstack)
{
    if (slotIndex == 0 && itemNameField != nullptr)
    {
        itemNameField->setText((itemstack == nullptr) ? "" : itemstack->getDisplayName());
        itemNameField->setEnabled(itemstack != nullptr);
        if (itemstack != nullptr && repairContainer != nullptr)
        {
            repairContainer->updateItemName(itemNameField->getText());
        }
    }
}

void GuiRepair::sendProgressBarUpdate(Container *, int_t, int_t)
{
}
