#include "GuiHopper.h"
#include "ContainerHopper.h"
#include "InventoryPlayer.h"
#include "IInventory.h"
#include "FontRenderer.h"
#include "RenderEngine.h"
#include "Minecraft.h"
#include "StatCollector.h"
#include "platform/RenderAPI.h"

GuiHopper::GuiHopper(InventoryPlayer *player, IInventory *hopperInv, EntityPlayer *entityPlayer)
	: GuiContainer(new ContainerHopper(player, hopperInv), true, (entityPlayer != nullptr) ? entityPlayer : (player != nullptr ? player->player : nullptr)),
	  playerInventory(player),
	  hopperInventory(hopperInv)
{
	ySize = 133;
}

void GuiHopper::drawGuiContainerForegroundLayer()
{
	if (hopperInventory != nullptr)
	{
		fontRenderer->drawString(StatCollector::translateToLocal(hopperInventory->getInvName()), 8, 6, 0x404040);
	}
	if (playerInventory != nullptr)
	{
		fontRenderer->drawString(StatCollector::translateToLocal(playerInventory->getInvName()), 8, (ySize - 96) + 2, 0x404040);
	}
}

void GuiHopper::drawGuiContainerBackgroundLayer(float_t partialTick)
{
	(void)partialTick;
	int_t tex = mc->renderEngine->getTexture("/gui/hopper.png");
	renderColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	mc->renderEngine->bindTexture(tex);
	int_t guiX = (width - xSize) / 2;
	int_t guiY = (height - ySize) / 2;
	drawTexturedModalRect(guiX, guiY, 0, 0, xSize, ySize);
}
