#include "GuiBeacon.h"
#include "ContainerBeacon.h"
#include "InventoryPlayer.h"
#include "TileEntityBeacon.h"
#include "RenderEngine.h"
#include "Minecraft.h"
#include "FontRenderer.h"
#include "StatCollector.h"
#include "GuiButton.h"
#include "Item.h"
#include "ItemStack.h"
GuiBeacon::GuiBeacon(InventoryPlayer *p, TileEntityBeacon *b, EntityPlayer *ep) : GuiContainer(new ContainerBeacon(p,b),true,ep?ep:(p?p->player:nullptr)),beacon(b) { xSize=230; ySize=219; }
void GuiBeacon::initGui(){GuiContainer::initGui();controlList.push_back(new GuiButton(-1,width/2+49,height/2+60,24,20,"OK"));controlList.push_back(new GuiButton(-2,width/2+75,height/2+60,24,20,"X"));buttonsNotDrawn=true;}
void GuiBeacon::updateScreen(){GuiContainer::updateScreen();if(beacon&&beacon->getLevels()>=0)buttonsNotDrawn=false;}
void GuiBeacon::actionPerformed(GuiButton*b){if(!b)return;if(b->id==-2){mc->displayGuiScreen(nullptr);return;}if(b->id==-1){mc->displayGuiScreen(nullptr);return;}GuiContainer::actionPerformed(b);}
void GuiBeacon::drawGuiContainerForegroundLayer(){fontRenderer->drawString(StatCollector::translateToLocal("tile.beacon.primary"),20,10,0xe0e0e0);fontRenderer->drawString(StatCollector::translateToLocal("tile.beacon.secondary"),130,10,0xe0e0e0);if(beacon){fontRenderer->drawString(std::to_string(beacon->getLevels()),20,30,0xffffff);fontRenderer->drawString(std::to_string(beacon->getPrimaryEffect()),20,45,0xffffff);fontRenderer->drawString(std::to_string(beacon->getSecondaryEffect()),130,45,0xffffff);}}
void GuiBeacon::drawGuiContainerBackgroundLayer(float_t){int t=mc->renderEngine->getTexture("/gui/beacon.png");mc->renderEngine->bindTexture(t);int x=(width-xSize)/2,y=(height-ySize)/2;drawTexturedModalRect(x,y,0,0,xSize,ySize);drawRect(x+42,y+109,x+130,y+125,0x66333333);}
