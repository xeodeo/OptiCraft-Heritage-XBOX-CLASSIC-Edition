#include "GuiRepair.h"
#include "ContainerRepair.h"
#include "InventoryPlayer.h"
#include "RenderEngine.h"
#include "Minecraft.h"
#include "FontRenderer.h"
#include "StatCollector.h"
GuiRepair::GuiRepair(InventoryPlayer *p, World *w, int x, int y, int z, EntityPlayer *ep) : GuiContainer(new ContainerRepair(p,w,x,y,z,ep?ep:(p?p->player:nullptr)),true,ep?ep:(p?p->player:nullptr)) { xSize=176; ySize=166; }
void GuiRepair::drawGuiContainerForegroundLayer() { fontRenderer->drawString(StatCollector::translateToLocal("container.repair"),60,6,0x404040); }
void GuiRepair::drawGuiContainerBackgroundLayer(float_t) { int t=mc->renderEngine->getTexture("/gui/repair.png"); mc->renderEngine->bindTexture(t); drawTexturedModalRect((width-xSize)/2,(height-ySize)/2,0,0,xSize,ySize); }
