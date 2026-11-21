#include "GuiBeacon.h"

#include "ContainerBeacon.h"
#include "EntityPlayer.h"
#include "EntityPlayerSP.h"
#include "FontRenderer.h"
#include "Item.h"
#include "ItemStack.h"
#include "Minecraft.h"
#include "Potion.h"
#include "RenderEngine.h"
#include "RenderHelper.h"
#include "RenderItem.h"
#include "StatCollector.h"
#include "TileEntityBeacon.h"
#include "platform/RenderAPI.h"

GuiBeaconButton::GuiBeaconButton(int id, int x, int y, const std::string &texture, int iconX, int iconY)
    : GuiButton(id, x, y, 22, 22, ""),
      buttonTexture(texture),
      field_82257_l(iconX),
      field_82258_m(iconY)
{
}

void GuiBeaconButton::drawButton(Minecraft *mc, int_t mouseX, int_t mouseY)
{
    if (!enabled2)
        return;

    mc->renderEngine->bindTexture(mc->renderEngine->getTexture("/gui/beacon.png"));
    renderColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    hovered = mouseX >= xPosition && mouseY >= yPosition && mouseX < xPosition + width && mouseY < yPosition + height;
    constexpr int textureY = 219;
    int textureX = 0;

    if (!enabled)
    {
        textureX += width * 2;
    }
    else if (field_82256_n)
    {
        textureX += width * 1;
    }
    else if (hovered)
    {
        textureX += width * 3;
    }

    drawTexturedModalRect(xPosition, yPosition, textureX, textureY, width, height);
    if (!buttonTexture.empty() && buttonTexture != "/gui/beacon.png")
    {
        mc->renderEngine->bindTexture(mc->renderEngine->getTexture(buttonTexture));
    }
    drawTexturedModalRect(xPosition + 2, yPosition + 2, field_82257_l, field_82258_m, 18, 18);
}

void GuiBeaconButton::func_82251_b(int, int)
{
}

GuiBeaconButtonPower::GuiBeaconButtonPower(GuiBeacon *gui, int id, int x, int y, int effectId, int tier)
    : GuiBeaconButton(id, x, y, "/gui/inventory.png",
                      (effectId >= 0 && effectId < 32 && Potion::potionTypes[effectId] != nullptr)
                          ? (Potion::potionTypes[effectId]->getStatusIconIndex() % 8 * 18) : 0,
                      (effectId >= 0 && effectId < 32 && Potion::potionTypes[effectId] != nullptr)
                          ? (198 + Potion::potionTypes[effectId]->getStatusIconIndex() / 8 * 18) : 0),
      beaconGui(gui),
      field_82261_l(effectId),
      field_82262_m(tier)
{
}

void GuiBeaconButtonPower::func_82251_b(int x, int y)
{
    if (field_82261_l < 0 || field_82261_l >= 32 || Potion::potionTypes[field_82261_l] == nullptr || beaconGui == nullptr)
        return;

    std::string text = StatCollector::translateToLocal(Potion::potionTypes[field_82261_l]->getName());
    if (field_82262_m >= 3 && Potion::regeneration != nullptr && field_82261_l != Potion::regeneration->id)
    {
        text += " II";
    }
    beaconGui->drawCreativeTabHoveringText(text, x, y);
}

GuiBeaconButtonConfirm::GuiBeaconButtonConfirm(GuiBeacon *gui, int id, int x, int y)
    : GuiBeaconButton(id, x, y, "/gui/beacon.png", 90, 220),
      beaconGui(gui)
{
}

void GuiBeaconButtonConfirm::func_82251_b(int x, int y)
{
    if (beaconGui != nullptr)
    {
        beaconGui->drawCreativeTabHoveringText(StatCollector::translateToLocal("gui.done"), x, y);
    }
}

GuiBeaconButtonCancel::GuiBeaconButtonCancel(GuiBeacon *gui, int id, int x, int y)
    : GuiBeaconButton(id, x, y, "/gui/beacon.png", 112, 220),
      beaconGui(gui)
{
}

void GuiBeaconButtonCancel::func_82251_b(int x, int y)
{
    if (beaconGui != nullptr)
    {
        beaconGui->drawCreativeTabHoveringText(StatCollector::translateToLocal("gui.cancel"), x, y);
    }
}

GuiBeacon::GuiBeacon(InventoryPlayer *playerInv, TileEntityBeacon *beaconTile, EntityPlayer *player)
    : GuiContainer(new ContainerBeacon(playerInv, beaconTile), true, player),
      beacon(beaconTile)
{
    xSize = 230;
    ySize = 219;
    if (beacon != nullptr)
    {
        pendingPrimaryEffect = beacon->getPrimaryEffect();
        pendingSecondaryEffect = beacon->getSecondaryEffect();
    }
}

void GuiBeacon::closeBeaconScreen()
{
    EntityPlayer *p = getContainerPlayer();
    if (p != nullptr)
    {
        p->closeScreen();
    }
    else if (mc != nullptr && mc->thePlayer != nullptr)
    {
        mc->thePlayer->closeScreen();
    }
    else if (mc != nullptr)
    {
        mc->displayGuiScreen(nullptr);
    }
}

void GuiBeacon::drawScreen(int_t mouseX, int_t mouseY, float_t partialTick)
{
    lastMouseX = mouseX;
    lastMouseY = mouseY;
    GuiContainer::drawScreen(mouseX, mouseY, partialTick);
}

void GuiBeacon::initGui()
{
    GuiContainer::initGui();

    int guiX = (width - xSize) / 2;
    int guiY = (height - ySize) / 2;

    beaconConfirmButton = new GuiBeaconButtonConfirm(this, -1, guiX + 164, guiY + 107);
    controlList.push_back(beaconConfirmButton);
    controlList.push_back(new GuiBeaconButtonCancel(this, -2, guiX + 190, guiY + 107));

    buttonsNotDrawn = true;
    beaconConfirmButton->enabled = false;
}

void GuiBeacon::updateScreen()
{
    GuiContainer::updateScreen();
    if (beacon == nullptr)
        return;

    int guiX = (width - xSize) / 2;
    int guiY = (height - ySize) / 2;

    if (buttonsNotDrawn && beacon->getLevels() >= 0)
    {
        buttonsNotDrawn = false;

        for (int tier = 0; tier <= 2; ++tier)
        {
            int count = TileEntityBeacon::effectsCount[tier];
            int totalW = count * 22 + (count - 1) * 2;

            for (int i = 0; i < count; ++i)
            {
                if (TileEntityBeacon::getEffect(tier, i) == nullptr)
                    continue;

                int effectId = TileEntityBeacon::getEffect(tier, i)->id;
                int btnX = guiX + 76 + i * 24 - totalW / 2;
                int btnY = guiY + 22 + tier * 25;
                GuiBeaconButtonPower *btn = new GuiBeaconButtonPower(this, (tier << 8) | effectId, btnX, btnY, effectId, tier);
                controlList.push_back(btn);

                if (tier >= beacon->getLevels())
                {
                    btn->enabled = false;
                }
                else if (effectId == pendingPrimaryEffect)
                {
                    btn->func_82254_b(true);
                }
            }
        }

        constexpr int tier3 = 3;
        int count3 = TileEntityBeacon::effectsCount[tier3] + 1;
        int totalW3 = count3 * 22 + (count3 - 1) * 2;

        for (int i = 0; i < count3 - 1; ++i)
        {
            if (TileEntityBeacon::getEffect(tier3, i) == nullptr)
                continue;

            int effectId = TileEntityBeacon::getEffect(tier3, i)->id;
            int btnX = guiX + 167 + i * 24 - totalW3 / 2;
            int btnY = guiY + 47;
            GuiBeaconButtonPower *btn = new GuiBeaconButtonPower(this, (tier3 << 8) | effectId, btnX, btnY, effectId, tier3);
            controlList.push_back(btn);

            if (tier3 >= beacon->getLevels())
            {
                btn->enabled = false;
            }
            else if (effectId == pendingSecondaryEffect)
            {
                btn->func_82254_b(true);
            }
        }

        if (pendingPrimaryEffect > 0)
        {
            int effectId = pendingPrimaryEffect;
            int btnX = guiX + 167 + (count3 - 1) * 24 - totalW3 / 2;
            int btnY = guiY + 47;
            GuiBeaconButtonPower *btn = new GuiBeaconButtonPower(this, (tier3 << 8) | effectId, btnX, btnY, effectId, tier3);
            controlList.push_back(btn);

            if (tier3 >= beacon->getLevels())
            {
                btn->enabled = false;
            }
            else if (pendingPrimaryEffect == pendingSecondaryEffect)
            {
                btn->func_82254_b(true);
            }
        }
    }

    if (beaconConfirmButton != nullptr)
    {
        ItemStack *pay = beacon->getStackInSlot(0);
        beaconConfirmButton->enabled = (pay != nullptr && pay->stackSize > 0 &&
                                        beacon->isStackValidForSlot(0, pay) &&
                                        pendingPrimaryEffect > 0);
    }
}

void GuiBeacon::actionPerformed(GuiButton *button)
{
    if (button == nullptr || beacon == nullptr)
        return;

    if (button->id == -2)
    {
        closeBeaconScreen();
    }
    else if (button->id == -1)
    {
        ContainerBeacon *container = dynamic_cast<ContainerBeacon *>(inventorySlots);
        if (container != nullptr && container->confirmSelection(pendingPrimaryEffect, pendingSecondaryEffect))
        {
            closeBeaconScreen();
        }
    }
    else
    {
        GuiBeaconButtonPower *powerBtn = dynamic_cast<GuiBeaconButtonPower *>(button);
        if (powerBtn != nullptr)
        {
            if (powerBtn->func_82255_b())
                return;

            int effectId = button->id & 0xFF;
            int tier = button->id >> 8;
            if (tier < 3)
            {
                // Stage locally instead of mutating the live TileEntityBeacon;
                // effects are committed only when Done is clicked with payment.
                const int oldPrimary = pendingPrimaryEffect;
                pendingPrimaryEffect = effectId;
                if (pendingSecondaryEffect == oldPrimary &&
                    (Potion::regeneration == nullptr || pendingSecondaryEffect != Potion::regeneration->id))
                {
                    pendingSecondaryEffect = 0;
                }
            }
            else
            {
                pendingSecondaryEffect = effectId;
            }

            // Java: buttonList.clear(). This button is the one being clicked:
            // clearControlList also drops GuiScreen's pointer to it.
            clearControlList();
            initGui();
            updateScreen();
        }
    }
}

void GuiBeacon::drawCreativeTabHoveringText(const std::string &text, int x, int y)
{
    if (fontRenderer == nullptr || text.empty())
        return;

    renderDisable(RenderCapability::RescaleNormal);
    RenderHelper::disableStandardItemLighting();
    renderDisable(RenderCapability::Lighting);
    renderDisable(RenderCapability::DepthTest);

    const int tooltipWidth = fontRenderer->getStringWidth(text);
    const int tooltipHeight = 8;
    int tx = x + 12;
    int ty = y - 12;

    if (guiLeft + tx + tooltipWidth + 6 > width)
    {
        tx -= 28 + tooltipWidth;
    }
    if (guiLeft + tx < 4)
    {
        tx = 4 - guiLeft;
    }
    if (guiTop + ty + tooltipHeight + 6 > height)
    {
        ty = height - guiTop - tooltipHeight - 6;
    }
    if (guiTop + ty < 4)
    {
        ty = 4 - guiTop;
    }

    zLevel = 300.0f;
    if (itemRenderer != nullptr)
        itemRenderer->zLevel = 300.0f;

    const int background = static_cast<int>(0xf0100010u);
    drawGradientRect(tx - 3, ty - 4, tx + tooltipWidth + 3, ty - 3, background, background);
    drawGradientRect(tx - 3, ty + tooltipHeight + 3, tx + tooltipWidth + 3, ty + tooltipHeight + 4, background, background);
    drawGradientRect(tx - 3, ty - 3, tx + tooltipWidth + 3, ty + tooltipHeight + 3, background, background);
    drawGradientRect(tx - 4, ty - 3, tx - 3, ty + tooltipHeight + 3, background, background);
    drawGradientRect(tx + tooltipWidth + 3, ty - 3, tx + tooltipWidth + 4, ty + tooltipHeight + 3, background, background);

    const int borderTop = 0x505000ff;
    const int borderBottom = (borderTop & 0x00fefefe) >> 1 | (borderTop & static_cast<int>(0xff000000u));
    drawGradientRect(tx - 3, ty - 2, tx - 2, ty + tooltipHeight + 2, borderTop, borderBottom);
    drawGradientRect(tx + tooltipWidth + 2, ty - 2, tx + tooltipWidth + 3, ty + tooltipHeight + 2, borderTop, borderBottom);
    drawGradientRect(tx - 3, ty - 3, tx + tooltipWidth + 3, ty - 2, borderTop, borderTop);
    drawGradientRect(tx - 3, ty + tooltipHeight + 2, tx + tooltipWidth + 3, ty + tooltipHeight + 3, borderBottom, borderBottom);

    fontRenderer->drawStringWithShadow(text, tx, ty, -1);

    zLevel = 0.0f;
    if (itemRenderer != nullptr)
        itemRenderer->zLevel = 0.0f;
    renderEnable(RenderCapability::Lighting);
    renderEnable(RenderCapability::DepthTest);
    RenderHelper::enableGUIStandardItemLighting();
    renderEnable(RenderCapability::RescaleNormal);
}

void GuiBeacon::drawGuiContainerForegroundLayer()
{
    RenderHelper::disableStandardItemLighting();
    drawCenteredString(fontRenderer, StatCollector::translateToLocal("tile.beacon.primary"), 62, 10, 0xE0E0E0);
    drawCenteredString(fontRenderer, StatCollector::translateToLocal("tile.beacon.secondary"), 169, 10, 0xE0E0E0);

    for (GuiButton *btn : controlList)
    {
        GuiBeaconButton *bBtn = dynamic_cast<GuiBeaconButton *>(btn);
        if (bBtn != nullptr && bBtn->enabled2 && bBtn->func_82252_a())
        {
            bBtn->func_82251_b(lastMouseX - guiLeft, lastMouseY - guiTop);
            break;
        }
    }

    RenderHelper::enableGUIStandardItemLighting();
}

void GuiBeacon::drawGuiContainerBackgroundLayer(float_t)
{
    renderColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    mc->renderEngine->bindTexture(mc->renderEngine->getTexture("/gui/beacon.png"));

    int guiX = (width - xSize) / 2;
    int guiY = (height - ySize) / 2;
    drawTexturedModalRect(guiX, guiY, 0, 0, xSize, ySize);

    if (itemRenderer != nullptr)
    {
        itemRenderer->zLevel = 100.0f;
        if (Item::emerald != nullptr)
        {
            ItemStack stack(Item::emerald);
            itemRenderer->renderItemIntoGUI(fontRenderer, mc->renderEngine, &stack, guiX + 42, guiY + 109);
        }
        if (Item::diamond != nullptr)
        {
            ItemStack stack(Item::diamond);
            itemRenderer->renderItemIntoGUI(fontRenderer, mc->renderEngine, &stack, guiX + 42 + 22, guiY + 109);
        }
        if (Item::ingotGold != nullptr)
        {
            ItemStack stack(Item::ingotGold);
            itemRenderer->renderItemIntoGUI(fontRenderer, mc->renderEngine, &stack, guiX + 42 + 44, guiY + 109);
        }
        if (Item::ingotIron != nullptr)
        {
            ItemStack stack(Item::ingotIron);
            itemRenderer->renderItemIntoGUI(fontRenderer, mc->renderEngine, &stack, guiX + 42 + 66, guiY + 109);
        }
        itemRenderer->zLevel = 0.0f;
    }
}
