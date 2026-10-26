#include "GuiRepair.h"
#include "ContainerRepair.h"
#include "Slot.h"
#include "InventoryPlayer.h"
#include "RenderEngine.h"
#include "Minecraft.h"
#include "FontRenderer.h"
#include "StatCollector.h"
#include "GuiTextField.h"
#include "ContainerRepair.h"
GuiRepair::GuiRepair(InventoryPlayer *p, World *w, int x, int y, int z, EntityPlayer *ep) : GuiContainer(new ContainerRepair(p,w,x,y,z,ep?ep:(p?p->player:nullptr)),true,ep?ep:(p?p->player:nullptr)) { xSize=176; ySize=166; }
GuiRepair::~GuiRepair(){delete itemNameField;}
void GuiRepair::initGui(){GuiContainer::initGui();itemNameField=new GuiTextField(this,fontRenderer,(width-xSize)/2+62,(height-ySize)/2+24,103,12,"");itemNameField->setMaxStringLength(30);itemNameField->setEnableBackgroundDrawing(false);itemNameField->setFocused(true);}
void GuiRepair::onGuiClosed(){if(itemNameField)itemNameField->setFocused(false);GuiContainer::onGuiClosed();}
void GuiRepair::keyTyped(char_t c, int_t key)
{
    if (itemNameField != nullptr && itemNameField->textboxKeyTyped(c, key)) { updateNameFromField(); return; }
    GuiContainer::keyTyped(c, key);
}
void GuiRepair::mouseClicked(int_t x, int_t y, int_t b)
{
    GuiContainer::mouseClicked(x, y, b);
    if (itemNameField != nullptr) itemNameField->mouseClicked(x, y, b);
}
void GuiRepair::updateNameFromField()
{
    ContainerRepair *repair = dynamic_cast<ContainerRepair *>(inventorySlots);
    if (repair != nullptr && itemNameField != nullptr) repair->updateItemName(itemNameField->getText());
}
void GuiRepair::drawScreen(int_t x, int_t y, float_t p)
{
    GuiContainer::drawScreen(x, y, p);
    if (itemNameField != nullptr) itemNameField->drawTextBox();
}
void GuiRepair::drawCostLabel(int cost)
{
    if (cost <= 0) return;
    int color = cost >= 40 ? 0xd04040 : 0x80c040;
    fontRenderer->drawString("Cost: " + std::to_string(cost), 100, 67, color);
}
void GuiRepair::drawInputHints(int left, int top)
{
    ContainerRepair *repair = dynamic_cast<ContainerRepair *>(inventorySlots);
    if (repair == nullptr) return;
    if (repair->getSlot(0)->getHasStack()) drawTexturedModalRect(left + 59, top + 20, 0, ySize, 110, 16);
    if ((repair->getSlot(0)->getHasStack() || repair->getSlot(1)->getHasStack()) && !repair->getSlot(2)->getHasStack())
        drawTexturedModalRect(left + 99, top + 45, xSize, 0, 28, 21);
}
void GuiRepair::drawGuiContainerForegroundLayer()
{
    fontRenderer->drawString(StatCollector::translateToLocal("container.repair"), 60, 6, 0x404040);
    ContainerRepair *repair = dynamic_cast<ContainerRepair *>(inventorySlots);
    if (repair != nullptr) drawCostLabel(repair->maximumCost);
}
void GuiRepair::drawGuiContainerBackgroundLayer(float_t)
{
    int texture = mc->renderEngine->getTexture("/gui/repair.png");
    mc->renderEngine->bindTexture(texture);
    int left = (width - xSize) / 2;
    int top = (height - ySize) / 2;
    drawTexturedModalRect(left, top, 0, 0, xSize, ySize);
    drawInputHints(left, top);
}
