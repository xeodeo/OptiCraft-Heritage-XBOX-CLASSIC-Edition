#include "GuiBeacon.h"
#include "ContainerBeacon.h"
#include "InventoryPlayer.h"
#include "TileEntityBeacon.h"
#include "RenderEngine.h"
#include "Minecraft.h"
#include "FontRenderer.h"
#include "StatCollector.h"
GuiBeacon::GuiBeacon(InventoryPlayer *p, TileEntityBeacon *b, EntityPlayer *ep) : GuiContainer(new ContainerBeacon(p,b),true,ep?ep:(p?p->player:nullptr)) { xSize=230; ySize=219; }
void GuiBeacon::drawGuiContainerForegroundLayer() { fontRenderer->drawString(StatCollector::translateToLocal("container.beacon"),10,6,0x404040); }
void GuiBeacon::drawGuiContainerBackgroundLayer(float_t) { int t=mc->renderEngine->getTexture("/gui/beacon.png"); mc->renderEngine->bindTexture(t); drawTexturedModalRect((width-xSize)/2,(height-ySize)/2,0,0,xSize,ySize); }
