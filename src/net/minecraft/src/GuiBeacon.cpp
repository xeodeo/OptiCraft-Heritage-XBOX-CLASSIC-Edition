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
void GuiBeacon::initGui()
{
    GuiContainer::initGui();
    int left = (width - xSize) / 2;
    int top = (height - ySize) / 2;
    controlList.push_back(new GuiButton(-1, left + 164, top + 107, 24, 20, "OK"));
    controlList.push_back(new GuiButton(-2, left + 190, top + 107, 24, 20, "X"));
    buttonsNotDrawn = true;
}
void GuiBeacon::updateScreen()
{
    GuiContainer::updateScreen();
    if (buttonsNotDrawn && beacon != nullptr && beacon->getLevels() >= 0)
        buttonsNotDrawn = false;
}
void GuiBeacon::actionPerformed(GuiButton *button)
{
    if (button == nullptr || !button->enabled) return;
    if (button->id == -1 || button->id == -2) { closeBeaconScreen(); return; }
    GuiContainer::actionPerformed(button);
}
void GuiBeacon::closeBeaconScreen()
{
    if (mc != nullptr) mc->displayGuiScreen(nullptr);
}
void GuiBeacon::drawEffectRow(int row, int selected, bool enabled)
{
    if (fontRenderer == nullptr) return;
    int y = 24 + row * 25;
    int color = enabled ? 0xe0e0e0 : 0x606060;
    fontRenderer->drawString("Effect " + std::to_string(row), 18, y, color);
    fontRenderer->drawString(std::to_string(selected), 90, y, color);
}
void GuiBeacon::drawPaymentHints(int x, int y)
{
    fontRenderer->drawString("Emerald", x, y, 0xa0a0a0);
    fontRenderer->drawString("Diamond", x + 22, y, 0xa0a0a0);
    fontRenderer->drawString("Gold", x + 44, y, 0xa0a0a0);
    fontRenderer->drawString("Iron", x + 66, y, 0xa0a0a0);
}
void GuiBeacon::drawSelectionSummary(int x, int y)
{
    if (beacon == nullptr) return;
    fontRenderer->drawString("Primary: " + std::to_string(beacon->getPrimaryEffect()), x, y, 0xffffff);
    fontRenderer->drawString("Secondary: " + std::to_string(beacon->getSecondaryEffect()), x, y + 12, 0xffffff);
}
void GuiBeacon::drawGuiContainerForegroundLayer()
{
    fontRenderer->drawString(StatCollector::translateToLocal("tile.beacon.primary"), 20, 10, 0xe0e0e0);
    fontRenderer->drawString(StatCollector::translateToLocal("tile.beacon.secondary"), 130, 10, 0xe0e0e0);
    if (beacon != nullptr) {
        drawEffectRow(0, beacon->getPrimaryEffect(), beacon->getLevels() >= 1);
        drawEffectRow(1, beacon->getSecondaryEffect(), beacon->getLevels() >= 2);
        drawSelectionSummary(20, 72);
    }
}
void GuiBeacon::drawGuiContainerBackgroundLayer(float_t)
{
    int texture = mc->renderEngine->getTexture("/gui/beacon.png");
    mc->renderEngine->bindTexture(texture);
    int left = (width - xSize) / 2;
    int top = (height - ySize) / 2;
    drawTexturedModalRect(left, top, 0, 0, xSize, ySize);
    drawRect(left + 42, top + 109, left + 130, top + 125, 0x66333333);
    drawPaymentHints(left + 42, top + 127);
}
