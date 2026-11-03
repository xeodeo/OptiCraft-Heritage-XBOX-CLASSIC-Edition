#include "GuiBeacon.h"

#include "ContainerBeacon.h"
#include "EntityPlayer.h"
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

    bool hovered = mouseX >= xPosition && mouseY >= yPosition && mouseX < xPosition + width && mouseY < yPosition + height;
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
                else if (effectId == beacon->getPrimaryEffect())
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
            else if (effectId == beacon->getSecondaryEffect())
            {
                btn->func_82254_b(true);
            }
        }

        if (beacon->getPrimaryEffect() > 0)
        {
            int effectId = beacon->getPrimaryEffect();
            int btnX = guiX + 167 + (count3 - 1) * 24 - totalW3 / 2;
            int btnY = guiY + 47;
            GuiBeaconButtonPower *btn = new GuiBeaconButtonPower(this, (tier3 << 8) | effectId, btnX, btnY, effectId, tier3);
            controlList.push_back(btn);

            if (tier3 >= beacon->getLevels())
            {
                btn->enabled = false;
            }
            else if (beacon->getPrimaryEffect() == beacon->getSecondaryEffect())
            {
                btn->func_82254_b(true);
            }
        }
    }

    if (beaconConfirmButton != nullptr)
    {
        beaconConfirmButton->enabled = (beacon->getStackInSlot(0) != nullptr && beacon->getPrimaryEffect() > 0);
    }
}

void GuiBeacon::actionPerformed(GuiButton *button)
{
    if (button == nullptr || beacon == nullptr)
        return;

    if (button->id == -2)
    {
        mc->displayGuiScreen(nullptr);
    }
    else if (button->id == -1)
    {
        if (beacon->getStackInSlot(0) != nullptr && beacon->getPrimaryEffect() > 0)
        {
            delete beacon->decrStackSize(0, 1);
        }
        mc->displayGuiScreen(nullptr);
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
                beacon->setPrimaryEffect(effectId);
            }
            else
            {
                beacon->setSecondaryEffect(effectId);
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
    int w = fontRenderer->getStringWidth(text);
    drawGradientRect(x - 3, y - 3, x + w + 3, y + 8 + 3, 0xC0000000, 0xC0000000);
    fontRenderer->drawStringWithShadow(text, x, y, -1);
}

void GuiBeacon::drawGuiContainerForegroundLayer()
{
    RenderHelper::disableStandardItemLighting();
    drawCenteredString(fontRenderer, StatCollector::translateToLocal("tile.beacon.primary"), 62, 10, 0xE0E0E0);
    drawCenteredString(fontRenderer, StatCollector::translateToLocal("tile.beacon.secondary"), 169, 10, 0xE0E0E0);

    int guiX = (width - xSize) / 2;
    int guiY = (height - ySize) / 2;

    for (GuiButton *btn : controlList)
    {
        GuiBeaconButton *bBtn = dynamic_cast<GuiBeaconButton *>(btn);
        if (bBtn != nullptr && bBtn->enabled2)
        {
            bBtn->func_82251_b(guiX, guiY);
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
