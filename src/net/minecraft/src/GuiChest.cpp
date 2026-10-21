#include "GuiChest.h"
#include "ContainerChest.h"
#include "IInventory.h"
#include "InventoryLargeChest.h"
#include "InventoryBasic.h"
#include "InventoryEnderChest.h"
#include "InventoryPlayer.h"
#include "FontRenderer.h"
#include "RenderEngine.h"
#include "Minecraft.h"
#include "StatCollector.h"
#include "platform/RenderAPI.h"

// The container frees the lower inventory when it was made just for this screen
// (a double chest pairing, a window the server sent). The ender chest inventory
// is an InventoryBasic too, but the player keeps it: freeing it here left the
// player pointing at freed memory and hung the next save or world load.
static bool ownsLowerInventory(IInventory *lower)
{
	if (dynamic_cast<InventoryEnderChest *>(lower) != nullptr)
		return false;
	return dynamic_cast<InventoryLargeChest *>(lower) != nullptr || dynamic_cast<InventoryBasic *>(lower) != nullptr;
}

GuiChest::GuiChest(IInventory *upper, IInventory *lower, EntityPlayer *player)
	: GuiContainer(new ContainerChest(upper, lower, ownsLowerInventory(lower)),
	               true,
	               (player != nullptr) ? player : (dynamic_cast<InventoryPlayer *>(upper) ? dynamic_cast<InventoryPlayer *>(upper)->player : nullptr))
	, upperChestInventory(upper)
	, lowerChestInventory(lower)
	, inventoryRows(0)
{
	field_948_f = false;
	inventoryRows = lower->getSizeInventory() / 9;
	ySize = 114 + inventoryRows * 18;
}

void GuiChest::drawGuiContainerForegroundLayer()
{
	fontRenderer->drawString(StatCollector::translateToLocal(lowerChestInventory->getInvName()), 8, 6, 0x404040);
	fontRenderer->drawString(StatCollector::translateToLocal(upperChestInventory->getInvName()), 8, (ySize - 96) + 2, 0x404040);
}

void GuiChest::drawGuiContainerBackgroundLayer(float_t partialTick)
{
	int_t tex = mc->renderEngine->getTexture("/gui/container.png");
	renderColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	mc->renderEngine->bindTexture(tex);
	int_t guiX = (width  - xSize) / 2;
	int_t guiY = (height - ySize) / 2;
	drawTexturedModalRect(guiX, guiY,                             0,   0,   xSize, inventoryRows * 18 + 17);
	drawTexturedModalRect(guiX, guiY + inventoryRows * 18 + 17,   0, 126,   xSize, 96);
}
