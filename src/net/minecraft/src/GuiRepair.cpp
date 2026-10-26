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
void GuiRepair::keyTyped(char_t c,int_t key){if(itemNameField&&itemNameField->textboxKeyTyped(c,key)){dynamic_cast<ContainerRepair*>(inventorySlots)->updateItemName(itemNameField->getText());return;}GuiContainer::keyTyped(c,key);}
void GuiRepair::mouseClicked(int_t x,int_t y,int_t b){GuiContainer::mouseClicked(x,y,b);if(itemNameField)itemNameField->mouseClicked(x,y,b);}
void GuiRepair::drawScreen(int_t x,int_t y,float_t p){GuiContainer::drawScreen(x,y,p);if(itemNameField)itemNameField->drawTextBox();}
void GuiRepair::drawGuiContainerForegroundLayer(){fontRenderer->drawString(StatCollector::translateToLocal("container.repair"),60,6,0x404040);ContainerRepair*c=dynamic_cast<ContainerRepair*>(inventorySlots);if(c&&c->maximumCost>0)fontRenderer->drawString(std::to_string(c->maximumCost),120,67,0x80c040);}
void GuiRepair::drawGuiContainerBackgroundLayer(float_t){int t=mc->renderEngine->getTexture("/gui/repair.png");mc->renderEngine->bindTexture(t);int x=(width-xSize)/2,y=(height-ySize)/2;drawTexturedModalRect(x,y,0,0,xSize,ySize);ContainerRepair*c=dynamic_cast<ContainerRepair*>(inventorySlots);if(c&&c->getSlot(0)->getHasStack())drawTexturedModalRect(x+59,y+20,0,ySize,110,16);}
