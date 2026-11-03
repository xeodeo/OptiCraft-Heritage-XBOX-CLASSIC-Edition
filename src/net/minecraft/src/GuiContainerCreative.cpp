#include "GuiContainerCreative.h"

#include <algorithm>
#include <cmath>

#include "AchievementList.h"
#include "ContainerCreative.h"
#include "CreativeCrafting.h"
#include "CreativeTabs.h"
#include "Enchantment.h"
#include "EntityPlayer.h"
#include "EntityPlayerSP.h"
#include "FontRenderer.h"
#include "GameSettings.h"
#include "GuiAchievements.h"
#include "GuiButton.h"
#include "GuiInventory.h"
#include "GuiStats.h"
#include "GuiTextField.h"
#include "InventoryBasic.h"
#include "InventoryPlayer.h"
#include "Item.h"
#include "ItemEnchantedBook.h"
#include "ItemStack.h"
#include "KeyBinding.h"
#include "Minecraft.h"
#include "PlayerController.h"
#include "RenderEngine.h"
#include "RenderHelper.h"
#include "RenderItem.h"
#include "RenderManager.h"
#include "Slot.h"
#include "SlotCreativeInventory.h"
#include "SoundManager.h"
#include "StringTranslate.h"
#include "net/minecraft/src/ControlIcon.h"
#include "pc/lwjgl/Keyboard.h"
#include "pc/lwjgl/Mouse.h"
#include "platform/Input.h"
#include "platform/PlatformConfig.h"
#include "platform/RenderAPI.h"

#if PLATFORM_PS2
#include "ps2/input/Ps2PadKeyCodes.h"
#include "ps2/input/Ps2PadState.h"
#endif

#if PLATFORM_XBOX
#include "xbox/input/XboxPad.h"
#endif

#if PLATFORM_HAS_VIRTUAL_KEYBOARD
#include "VirtualKeyboard.h"
#endif

InventoryBasic GuiContainerCreative::inventory("tmp", 45, false);
int GuiContainerCreative::selectedTabIndex = 0;
ItemStack *GuiContainerCreative::s_tabIcons[12] = { nullptr };
RenderItem *GuiContainerCreative::creativeItemRenderer = new RenderItem();

GuiContainerCreative::GuiContainerCreative(EntityPlayer *player)
    : GuiContainer(new ContainerCreative(player), true, player)
    , currentScroll(0.0f)
    , isScrolling(false)
    , wasClicking(false)
    , searchField(nullptr)
    , destroyItemSlot(nullptr)
    , field_74234_w(false)
    , creativeCrafting(nullptr)
    , xSize_lo(0.0f)
    , ySize_lo(0.0f)
{
    player->craftingInventory = inventorySlots;
    field_948_f = true;
    player->addStat(AchievementList::openInventory, 1);
    xSize = 195;
    ySize = 136;
}

GuiContainerCreative::~GuiContainerCreative()
{
    delete searchField;
    searchField = nullptr;

    delete creativeCrafting;
    creativeCrafting = nullptr;

    ContainerCreative *container = static_cast<ContainerCreative *>(inventorySlots);
    if (container != nullptr && !backupContainerSlots.empty())
    {
        for (Slot *s : container->slots)
        {
            if (s != destroyItemSlot)
                delete s;
        }
        delete destroyItemSlot;
        destroyItemSlot = nullptr;

        container->slots = backupContainerSlots;
        backupContainerSlots.clear();
    }
}

InventoryBasic *GuiContainerCreative::getInventory()
{
    return &inventory;
}

void GuiContainerCreative::updateScreen()
{
    if (mc != nullptr && mc->playerController != nullptr && !mc->playerController->isInCreativeMode())
    {
        EntityPlayer *p = getContainerPlayer();
        if (p == nullptr && mc != nullptr) p = mc->thePlayer;
        if (mc != nullptr && mc->isSplitScreenActive())
            mc->displayPlayerScreen(getOwnerPlayerIndex(), new GuiInventory(p ? p : mc->thePlayer));
        else
            mc->displayGuiScreen(new GuiInventory(p ? p : mc->thePlayer));
        return;
    }

    if (searchField != nullptr)
        searchField->updateCursorCounter();

    if (platformTextInputExclusive())
        return;

#if PLATFORM_PS2
    const int pIdx = getOwnerPlayerIndex();
    const Ps2PadSnapshot &pad = ps2PadGetSnapshot(pIdx);
    if (pad.connected)
    {
        if (pad.pressed & PS2_PAD_L1)
        {
            setCurrentCreativeTab(CreativeTabs::creativeTabArray[(selectedTabIndex + 11) % 12]);
            if (mc != nullptr && mc->sndManager != nullptr)
                mc->sndManager->playSoundFX("random.click", 1.0f, 1.0f);
        }
        else if (pad.pressed & PS2_PAD_R1)
        {
            setCurrentCreativeTab(CreativeTabs::creativeTabArray[(selectedTabIndex + 1) % 12]);
            if (mc != nullptr && mc->sndManager != nullptr)
                mc->sndManager->playSoundFX("random.click", 1.0f, 1.0f);
        }

        if (pad.pressed & PS2_PAD_SQUARE)
        {
            EntityPlayer *p = getContainerPlayer();
            if (p == nullptr && mc != nullptr) p = mc->thePlayer;
            if (p != nullptr && p->inventory != nullptr && p->inventory->getItemStack() != nullptr)
            {
                delete p->inventory->getItemStack();
                p->inventory->setItemStack(nullptr);
                if (mc != nullptr && mc->sndManager != nullptr)
                    mc->sndManager->playSoundFX("random.pop", 0.6f, 0.8f);
            }
        }
    }
#elif PLATFORM_XBOX
    const PlatformTextInputSnapshot pad = platformTextInputSnapshot(platformMenuPad());
    const unsigned short pagePressed = XboxPad::consumePagePressed();
    if (pad.connected)
    {
        if (pagePressed & XBOX_PAD_WHITE)
        {
            setCurrentCreativeTab(CreativeTabs::creativeTabArray[(selectedTabIndex + 11) % 12]);
            if (mc != nullptr && mc->sndManager != nullptr)
                mc->sndManager->playSoundFX("random.click", 1.0f, 1.0f);
        }
        else if (pagePressed & XBOX_PAD_BLACK)
        {
            setCurrentCreativeTab(CreativeTabs::creativeTabArray[(selectedTabIndex + 1) % 12]);
            if (mc != nullptr && mc->sndManager != nullptr)
                mc->sndManager->playSoundFX("random.click", 1.0f, 1.0f);
        }

        if (pad.pressed & PLATFORM_TEXT_SHIFT)
        {
            EntityPlayer *p = getContainerPlayer();
            if (p == nullptr && mc != nullptr) p = mc->thePlayer;
            if (p != nullptr && p->inventory != nullptr && p->inventory->getItemStack() != nullptr)
            {
                delete p->inventory->getItemStack();
                p->inventory->setItemStack(nullptr);
                if (mc != nullptr && mc->sndManager != nullptr)
                    mc->sndManager->playSoundFX("random.pop", 0.6f, 0.8f);
            }
        }
    }
#else
    const PlatformTextInputSnapshot pad = platformTextInputSnapshot(getOwnerPlayerIndex());
    if (pad.connected)
    {
        if (pad.pressed & PLATFORM_TEXT_PREV_PAGE)
        {
            setCurrentCreativeTab(CreativeTabs::creativeTabArray[(selectedTabIndex + 11) % 12]);
            if (mc != nullptr && mc->sndManager != nullptr)
                mc->sndManager->playSoundFX("random.click", 1.0f, 1.0f);
        }
        else if (pad.pressed & PLATFORM_TEXT_NEXT_PAGE)
        {
            setCurrentCreativeTab(CreativeTabs::creativeTabArray[(selectedTabIndex + 1) % 12]);
            if (mc != nullptr && mc->sndManager != nullptr)
                mc->sndManager->playSoundFX("random.click", 1.0f, 1.0f);
        }

        if (pad.pressed & (PLATFORM_TEXT_SPACE | PLATFORM_TEXT_SHIFT))
        {
            EntityPlayer *p = getContainerPlayer();
            if (p == nullptr && mc != nullptr) p = mc->thePlayer;
            if (p != nullptr && p->inventory != nullptr && p->inventory->getItemStack() != nullptr)
            {
                delete p->inventory->getItemStack();
                p->inventory->setItemStack(nullptr);
                if (mc != nullptr && mc->sndManager != nullptr)
                    mc->sndManager->playSoundFX("random.pop", 0.6f, 0.8f);
            }
        }
    }
#endif

    GuiContainer::updateScreen();
}

void GuiContainerCreative::initGui()
{
#if PLATFORM_XBOX
    (void)XboxPad::consumePagePressed();
#endif
    if (mc != nullptr && mc->playerController != nullptr && !mc->playerController->isInCreativeMode())
    {
        EntityPlayer *p = getContainerPlayer();
        if (p == nullptr && mc != nullptr) p = mc->thePlayer;
        if (mc != nullptr && mc->isSplitScreenActive())
            mc->displayPlayerScreen(getOwnerPlayerIndex(), new GuiInventory(p ? p : mc->thePlayer));
        else
            mc->displayGuiScreen(new GuiInventory(p ? p : mc->thePlayer));
        return;
    }

    GuiContainer::initGui();
    for (GuiButton *button : controlList)
        delete button;
    controlList.clear();

    lwjgl::Keyboard::enableRepeatEvents(true);

    delete searchField;
    searchField = new GuiTextField(this, fontRenderer, guiLeft + 82, guiTop + 6, 89, 12, "");
    searchField->setMaxStringLength(15);
    searchField->setEnableBackgroundDrawing(false);
    searchField->setEnabled(false);
    searchField->setTextColor(0xffffff);

    int tabIdx = selectedTabIndex;
    selectedTabIndex = -1;
    if (tabIdx < 0 || tabIdx >= 12)
        tabIdx = 0;
    setCurrentCreativeTab(CreativeTabs::creativeTabArray[tabIdx]);

    EntityPlayer *p = getContainerPlayer();
    if (p == nullptr && mc != nullptr) p = mc->thePlayer;
    if (p != nullptr && p->inventorySlots != nullptr)
    {
        delete creativeCrafting;
        creativeCrafting = new CreativeCrafting(mc);
        p->inventorySlots->addCraftingToCrafters(creativeCrafting);
    }
}

void GuiContainerCreative::onGuiClosed()
{
    GuiContainer::onGuiClosed();

    EntityPlayer *p = getContainerPlayer();
    if (p == nullptr && mc != nullptr) p = mc->thePlayer;
    if (p != nullptr && p->inventorySlots != nullptr && creativeCrafting != nullptr)
    {
        p->inventorySlots->removeCraftingFromCrafters(creativeCrafting);
        delete creativeCrafting;
        creativeCrafting = nullptr;
    }

    if (searchField != nullptr)
    {
        searchField->setFocused(false);
    }

    lwjgl::Keyboard::enableRepeatEvents(false);
}

void GuiContainerCreative::handleMouseClick(Slot *slot, int_t slotId, int_t button, bool shift)
{
    field_74234_w = true;

    EntityPlayer *player = getContainerPlayer();
    if (player == nullptr && mc != nullptr) player = mc->thePlayer;
    if (player == nullptr || player->inventory == nullptr) return;
    InventoryPlayer *playerInventory = player->inventory;

    if (slot == nullptr && selectedTabIndex != CreativeTabs::tabInventory->getTabIndex())
    {
        ItemStack *held = playerInventory->getItemStack();
        if (held != nullptr)
        {
            if (button == 0)
            {
                player->dropPlayerItem(held);
                mc->playerController->sendPacketDropItem(held);
                playerInventory->setItemStack(nullptr);
            }
            else if (button == 1)
            {
                ItemStack *dropped = held->splitStack(1);
                player->dropPlayerItem(dropped);
                mc->playerController->sendPacketDropItem(dropped);
                if (held->stackSize == 0)
                {
                    delete held;
                    playerInventory->setItemStack(nullptr);
                }
            }
        }
    }
    else if (slot == destroyItemSlot && shift)
    {
        for (int_t i = 0; i < (int_t)player->inventorySlots->slots.size(); ++i)
        {
            mc->playerController->sendSlotPacket(nullptr, i);
        }
    }
    else if (selectedTabIndex == CreativeTabs::tabInventory->getTabIndex())
    {
        if (slot == destroyItemSlot)
        {
            if (playerInventory->getItemStack() != nullptr)
            {
                delete playerInventory->getItemStack();
                playerInventory->setItemStack(nullptr);
            }
        }
        else
        {
            Slot *realSlot = slot;
            SlotCreativeInventory *sci = dynamic_cast<SlotCreativeInventory *>(slot);
            if (sci != nullptr)
                realSlot = sci->theSlot;
            int_t realSlotNumber = (realSlot != nullptr) ? realSlot->slotNumber : slotId;
            player->inventorySlots->slotClick(realSlotNumber, button, shift, player);
            player->inventorySlots->detectAndSendChanges();
        }
    }
    else if (slot != nullptr && slot->getInventory() == &inventory)
    {
        ItemStack *held = playerInventory->getItemStack();
        ItemStack *listed = slot->getStack();

        if (button == 2)
        {
            if (listed != nullptr && button >= 0 && button < 9)
            {
                ItemStack *copy = ItemStack::copyItemStack(listed);
                if (copy != nullptr)
                {
                    copy->stackSize = copy->getMaxStackSize();
                    player->inventory->setInventorySlotContents(button, copy);
                    player->inventorySlots->detectAndSendChanges();
                }
            }
            return;
        }

        if (held != nullptr && listed != nullptr && held->isItemEqual(listed))
        {
            if (button == 0)
            {
                if (shift)
                    held->stackSize = held->getMaxStackSize();
                else if (held->stackSize < held->getMaxStackSize())
                    held->stackSize++;
            }
            else if (held->stackSize <= 1)
            {
                delete held;
                playerInventory->setItemStack(nullptr);
            }
            else
            {
                held->stackSize--;
            }
        }
        else if (listed != nullptr && held == nullptr)
        {
            ItemStack *copy = ItemStack::copyItemStack(listed);
            if (copy != nullptr)
            {
                if (shift)
                    copy->stackSize = copy->getMaxStackSize();
                playerInventory->setItemStack(copy);
            }
        }
        else
        {
            if (held != nullptr)
                delete held;
            playerInventory->setItemStack(nullptr);
        }
    }
    else
    {
        inventorySlots->slotClick(slot == nullptr ? slotId : slot->slotNumber, button, shift, player);
        if (slot != nullptr)
        {
            ItemStack *stack = inventorySlots->getSlot(slot->slotNumber)->getStack();
            int_t packetSlot = slot->slotNumber - (int_t)inventorySlots->slots.size() + 9 + 36;
            mc->playerController->sendSlotPacket(stack, packetSlot);
        }
    }
}

void GuiContainerCreative::keyTyped(char_t c, int_t key)
{
    if (key == 1 || (mc != nullptr && mc->gameSettings != nullptr && mc->gameSettings->keyBindInventory != nullptr && key == mc->gameSettings->keyBindInventory->keyCode))
    {
        EntityPlayer *p = getContainerPlayer();
        if (p == nullptr && mc != nullptr) p = mc->thePlayer;
        if (p != nullptr)
            p->closeScreen();
        return;
    }

    if (selectedTabIndex != CreativeTabs::tabAllSearch->getTabIndex())
    {
        if (mc != nullptr && mc->gameSettings != nullptr && mc->gameSettings->keyBindChat != nullptr &&
            mc->gameSettings->keyBindChat->keyCode > 0 && key == mc->gameSettings->keyBindChat->keyCode)
        {
            setCurrentCreativeTab(CreativeTabs::tabAllSearch);
        }
        else if (key == lwjgl::Keyboard::KEY_PRIOR || key == lwjgl::Keyboard::KEY_Q)
        {
            setCurrentCreativeTab(CreativeTabs::creativeTabArray[(selectedTabIndex + 11) % 12]);
            if (mc != nullptr && mc->sndManager != nullptr)
                mc->sndManager->playSoundFX("random.click", 1.0f, 1.0f);
        }
        else if (key == lwjgl::Keyboard::KEY_NEXT || key == lwjgl::Keyboard::KEY_TAB)
        {
            setCurrentCreativeTab(CreativeTabs::creativeTabArray[(selectedTabIndex + 1) % 12]);
            if (mc != nullptr && mc->sndManager != nullptr)
                mc->sndManager->playSoundFX("random.click", 1.0f, 1.0f);
        }
        else if (key == lwjgl::Keyboard::KEY_X || key == lwjgl::Keyboard::KEY_DELETE)
        {
            EntityPlayer *p = getContainerPlayer();
            if (p == nullptr && mc != nullptr) p = mc->thePlayer;
            if (p != nullptr && p->inventory != nullptr && p->inventory->getItemStack() != nullptr)
            {
                delete p->inventory->getItemStack();
                p->inventory->setItemStack(nullptr);
                if (mc != nullptr && mc->sndManager != nullptr)
                    mc->sndManager->playSoundFX("random.pop", 0.6f, 0.8f);
            }
        }
        else
        {
            GuiContainer::keyTyped(c, key);
        }
    }
    else
    {
        if (field_74234_w)
        {
            field_74234_w = false;
            if (searchField != nullptr)
                searchField->setText("");
        }

        if (searchField != nullptr && searchField->textboxKeyTyped(c, key))
        {
            updateCreativeSearch();
        }
        else
        {
            GuiContainer::keyTyped(c, key);
        }
    }
}

void GuiContainerCreative::updateCreativeSearch()
{
    ContainerCreative *container = static_cast<ContainerCreative *>(inventorySlots);
    if (container == nullptr)
        return;
    container->clearItemList();

    for (int i = 0; i < Item::ITEM_LIST_SIZE; ++i)
    {
        Item *item = Item::itemsList[i];
        if (item != nullptr && item->getCreativeTab() != nullptr)
        {
            item->getSubItems(item->shiftedIndex, nullptr, container->itemList);
        }
    }

    if (Item::enchantedBook != nullptr)
    {
        ItemEnchantedBook *enchantedBookItem = dynamic_cast<ItemEnchantedBook *>(Item::enchantedBook);
        if (enchantedBookItem != nullptr)
        {
            for (int i = 0; i < 256; ++i)
            {
                Enchantment *ench = Enchantment::enchantmentsList[i];
                if (ench != nullptr)
                {
                    enchantedBookItem->func_92113_a(ench, container->itemList);
                }
            }
        }
    }

    if (searchField != nullptr)
    {
        std::string query = searchField->getText();
        std::transform(query.begin(), query.end(), query.begin(), ::tolower);

        if (!query.empty())
        {
            std::vector<ItemStack *> filtered;
            for (ItemStack *stack : container->itemList)
            {
                bool match = false;
                if (stack != nullptr)
                {
                    std::string displayName = stack->getDisplayName();
                    std::transform(displayName.begin(), displayName.end(), displayName.begin(), ::tolower);
                    if (displayName.find(query) != std::string::npos)
                    {
                        match = true;
                    }
                    else if (stack->getItem() != nullptr)
                    {
                        std::vector<std::string> tooltip;
                        stack->getItem()->addInformation(stack, tooltip);
                        for (std::string tip : tooltip)
                        {
                            std::transform(tip.begin(), tip.end(), tip.begin(), ::tolower);
                            if (tip.find(query) != std::string::npos)
                            {
                                match = true;
                                break;
                            }
                        }
                    }
                }
                if (match)
                {
                    filtered.push_back(stack);
                }
                else
                {
                    delete stack;
                }
            }
            container->itemList = filtered;
        }
    }

    currentScroll = 0.0f;
    container->scrollTo(0.0f);
}

void GuiContainerCreative::setCurrentCreativeTab(CreativeTabs *tab)
{
    if (tab == nullptr)
        return;

    int prevTab = selectedTabIndex;
    selectedTabIndex = tab->getTabIndex();
    ContainerCreative *container = static_cast<ContainerCreative *>(inventorySlots);
    if (container == nullptr)
        return;

    container->clearItemList();
    tab->displayAllReleventItems(container->itemList);

    EntityPlayer *player = getContainerPlayer();
    if (player == nullptr && mc != nullptr) player = mc->thePlayer;

    if (tab == CreativeTabs::tabInventory)
    {
        if (player != nullptr && player->inventorySlots != nullptr)
        {
            Container *playerContainer = player->inventorySlots;
            if (backupContainerSlots.empty())
            {
                backupContainerSlots = container->slots;
            }
            container->slots.clear();

            for (size_t i = 0; i < playerContainer->slots.size(); ++i)
            {
                SlotCreativeInventory *creativeSlot = new SlotCreativeInventory(this, playerContainer->slots[i], (int)i);
                container->slots.push_back(creativeSlot);
                if (i >= 5 && i < 9)
                {
                    int armorIdx = (int)i - 5;
                    int col = armorIdx / 2;
                    int row = armorIdx % 2;
                    creativeSlot->xDisplayPosition = 9 + col * 54;
                    creativeSlot->yDisplayPosition = 6 + row * 27;
                }
                else if (i < 5)
                {
                    creativeSlot->xDisplayPosition = -2000;
                    creativeSlot->yDisplayPosition = -2000;
                }
                else if (i < playerContainer->slots.size())
                {
                    int invIdx = (int)i - 9;
                    int col = invIdx % 9;
                    int row = invIdx / 9;
                    creativeSlot->xDisplayPosition = 9 + col * 18;
                    if (i >= 36)
                    {
                        creativeSlot->yDisplayPosition = 112;
                    }
                    else
                    {
                        creativeSlot->yDisplayPosition = 54 + row * 18;
                    }
                }
            }

            delete destroyItemSlot;
            destroyItemSlot = new Slot(&inventory, 0, 173, 112);
            container->slots.push_back(destroyItemSlot);
        }
    }
    else if (prevTab == CreativeTabs::tabInventory->getTabIndex())
    {
        for (Slot *s : container->slots)
        {
            if (s != destroyItemSlot)
                delete s;
        }
        delete destroyItemSlot;
        destroyItemSlot = nullptr;

        container->slots = backupContainerSlots;
        backupContainerSlots.clear();
    }

    if (searchField != nullptr)
    {
        if (tab == CreativeTabs::tabAllSearch)
        {
            searchField->setEnabled(true);
            searchField->setCanLoseFocus(false);
#if PLATFORM_HAS_VIRTUAL_KEYBOARD
            // Consoles: focusing opens the on-screen keyboard, which the user
            // wants only when the search bar is clicked, not on entering the tab.
            searchField->setFocused(false);
#else
            searchField->setFocused(true);
#endif
            searchField->setText("");
            updateCreativeSearch();
        }
        else
        {
            searchField->setEnabled(false);
            searchField->setCanLoseFocus(true);
            searchField->setFocused(false);
        }
    }

    currentScroll = 0.0f;
    container->scrollTo(0.0f);
}

void GuiContainerCreative::mouseClicked(int_t mouseX, int_t mouseY, int_t button)
{
    if (button == 0)
    {
        int_t relX = mouseX - guiLeft;
        int_t relY = mouseY - guiTop;

        for (int i = 0; i < 12; ++i)
        {
            CreativeTabs *tab = CreativeTabs::creativeTabArray[i];
            if (tab != nullptr && func_74232_a(tab, relX, relY))
            {
                return;
            }
        }
    }

    if (searchField != nullptr)
    {
        searchField->mouseClicked(mouseX, mouseY, button);
    }

    GuiContainer::mouseClicked(mouseX, mouseY, button);
}

void GuiContainerCreative::mouseMovedOrUp(int_t mouseX, int_t mouseY, int_t button)
{
    if (button == 0)
    {
        int_t relX = mouseX - guiLeft;
        int_t relY = mouseY - guiTop;

        for (int i = 0; i < 12; ++i)
        {
            CreativeTabs *tab = CreativeTabs::creativeTabArray[i];
            if (tab != nullptr && func_74232_a(tab, relX, relY))
            {
                setCurrentCreativeTab(tab);
                if (mc != nullptr && mc->sndManager != nullptr)
                    mc->sndManager->playSoundFX("random.click", 1.0f, 1.0f);
                return;
            }
        }
    }

    GuiContainer::mouseMovedOrUp(mouseX, mouseY, button);
}

bool GuiContainerCreative::func_74232_a(CreativeTabs *tab, int_t mouseX, int_t mouseY)
{
    int_t col = tab->getTabColumn();
    int_t tabX = 28 * col;
    int_t tabY = 0;
    if (col == 5)
        tabX = xSize - 28 + 2;
    else if (col > 0)
        tabX += col;

    if (tab->isTabInFirstRow())
        tabY -= 32;
    else
        tabY += ySize;

    return mouseX >= tabX && mouseX <= tabX + 28 && mouseY >= tabY && mouseY <= tabY + 32;
}

bool GuiContainerCreative::renderCreativeInventoryHoveringText(CreativeTabs *tab, int_t mouseX, int_t mouseY)
{
    int_t col = tab->getTabColumn();
    int_t tabX = 28 * col;
    int_t tabY = 0;
    if (col == 5)
        tabX = xSize - 28 + 2;
    else if (col > 0)
        tabX += col;

    if (tab->isTabInFirstRow())
        tabY -= 32;
    else
        tabY += ySize;

    if (isPointInRegion(tabX + 3, tabY + 3, 23, 27, mouseX, mouseY))
    {
        drawCreativeTabHoveringText(tab->getTranslatedTabLabel(), mouseX, mouseY);
        return true;
    }
    return false;
}

bool GuiContainerCreative::isPointInRegion(int_t x, int_t y, int_t w, int_t h, int_t pointX, int_t pointY)
{
    pointX -= guiLeft;
    pointY -= guiTop;
    return pointX >= x - 1 && pointX < x + w + 1 && pointY >= y - 1 && pointY < y + h + 1;
}

void GuiContainerCreative::drawCreativeTabHoveringText(const std::string &text, int_t x, int_t y)
{
    if (text.empty() || fontRenderer == nullptr)
        return;

    int_t textW = fontRenderer->getStringWidth(text);
    int_t tx = x + 12;
    int_t ty = y - 12;
    if (tx + textW + 6 > width)
        tx = width - textW - 6;
    if (ty < 4)
        ty = 4;

    // Java GuiContainer.drawCreativeTabHoveringText: no depth test or lighting,
    // so the tab icons (drawn with depth) cannot cover the label.
    renderDisable(RenderCapability::RescaleNormal);
    RenderHelper::disableStandardItemLighting();
    renderDisable(RenderCapability::Lighting);
    renderDisable(RenderCapability::DepthTest);
    renderClear(RenderClearMask::Depth);
    drawGradientRect(tx - 3, ty - 3, tx + textW + 3, ty + 11, 0xf0100010, 0xf0100010);
    drawGradientRect(tx - 4, ty - 2, tx - 3, ty + 10, 0x505000ff, 0x5028007f);
    drawGradientRect(tx + textW + 3, ty - 2, tx + textW + 4, ty + 10, 0x505000ff, 0x5028007f);
    drawGradientRect(tx - 3, ty - 4, tx + textW + 3, ty - 3, 0x505000ff, 0x505000ff);
    drawGradientRect(tx - 3, ty + 10, tx + textW + 3, ty + 11, 0x5028007f, 0x5028007f);
    fontRenderer->drawStringWithShadow(text, tx, ty, 0xffffffff);
}

void GuiContainerCreative::handleMouseInput()
{
    GuiContainer::handleMouseInput();
    int_t wheel = lwjgl::Mouse::getEventDWheel();
    if (wheel != 0 && needsScrollBars())
    {
        ContainerCreative *container = static_cast<ContainerCreative *>(inventorySlots);
        int_t rows = (int_t)container->itemList.size() / 9 - 5 + 1;
        int_t dir = wheel > 0 ? 1 : -1;
        currentScroll = (float_t)((double)currentScroll - (double)dir / (double)rows);
        if (currentScroll < 0.0f) currentScroll = 0.0f;
        if (currentScroll > 1.0f) currentScroll = 1.0f;
        container->scrollTo(currentScroll);
    }
}

bool GuiContainerCreative::scrollRows(int_t direction)
{
    if (direction == 0 || inventorySlots == nullptr || !needsScrollBars())
        return false;

    ContainerCreative *container = static_cast<ContainerCreative *>(inventorySlots);
    const int_t rows = (int_t)container->itemList.size() / 9 - 5 + 1;
    if (rows <= 0)
        return false;

    const int_t firstRow = (int_t)((double)(currentScroll * (float_t)rows) + 0.5);
    int_t targetRow = firstRow + direction;
    if (targetRow < 0) targetRow = 0;
    if (targetRow > rows) targetRow = rows;
    if (targetRow == firstRow)
        return false;

    currentScroll = (float_t)targetRow / (float_t)rows;
    container->scrollTo(currentScroll);
    return true;
}

bool GuiContainerCreative::needsScrollBars()
{
    return selectedTabIndex != CreativeTabs::tabInventory->getTabIndex()
        && selectedTabIndex >= 0 && selectedTabIndex < 12
        && CreativeTabs::creativeTabArray[selectedTabIndex]->shouldHidePlayerInventory()
        && static_cast<ContainerCreative *>(inventorySlots)->hasMoreThan1PageOfItemsInList();
}

Slot *GuiContainerCreative::getControllerNavigationTarget(Slot *selected, int_t dirX, int_t dirY)
{
    if (selected == nullptr || inventorySlots == nullptr)
        return nullptr;

    if (selectedTabIndex == CreativeTabs::tabInventory->getTabIndex())
        return nullptr;

    const int_t selectedIndex = selected->slotNumber;
    if (selectedIndex >= 0 && selectedIndex < 45)
    {
        const int_t col = selectedIndex % 9;
        const int_t row = selectedIndex / 9;

        if (dirX != 0)
        {
            int_t targetCol = col + dirX;
            if (targetCol < 0 || targetCol >= 9)
                return selected;
            return inventorySlots->slots[row * 9 + targetCol];
        }

        if (dirY < 0)
        {
            if (row > 0)
                return inventorySlots->slots[(row - 1) * 9 + col];
            scrollRows(-1);
            return selected;
        }

        if (dirY > 0)
        {
            if (row < 4)
                return inventorySlots->slots[(row + 1) * 9 + col];
            if (scrollRows(1))
                return selected;
            const int_t hotbarIdx = 45 + col;
            if (hotbarIdx < (int_t)inventorySlots->slots.size())
                return inventorySlots->slots[hotbarIdx];
            return selected;
        }
    }
    else if (selectedIndex >= 45 && selectedIndex < 54)
    {
        const int_t col = selectedIndex - 45;
        if (dirX != 0)
        {
            int_t targetCol = col + dirX;
            if (targetCol < 0 || targetCol >= 9)
                return selected;
            return inventorySlots->slots[45 + targetCol];
        }

        if (dirY < 0)
        {
            return inventorySlots->slots[4 * 9 + col];
        }
    }

    return nullptr;
}

void GuiContainerCreative::drawScreen(int_t mouseX, int_t mouseY, float_t partialTick)
{
    xSize_lo = (float_t)mouseX;
    ySize_lo = (float_t)mouseY;

    bool clicking = lwjgl::Mouse::isButtonDown(0);
    int_t scrollLeft = guiLeft + 175;
    int_t scrollTop = guiTop + 18;
    int_t scrollRight = scrollLeft + 14;
    int_t scrollBottom = scrollTop + 112;

    if (!wasClicking && clicking && mouseX >= scrollLeft && mouseY >= scrollTop
        && mouseX < scrollRight && mouseY < scrollBottom)
    {
        isScrolling = needsScrollBars();
    }
    if (!clicking)
        isScrolling = false;
    wasClicking = clicking;

    if (isScrolling)
    {
        currentScroll = ((float_t)(mouseY - scrollTop) - 7.5f) / ((float_t)(scrollBottom - scrollTop) - 15.0f);
        if (currentScroll < 0.0f) currentScroll = 0.0f;
        if (currentScroll > 1.0f) currentScroll = 1.0f;
        static_cast<ContainerCreative *>(inventorySlots)->scrollTo(currentScroll);
    }

    GuiContainer::drawScreen(mouseX, mouseY, partialTick);

    for (int i = 0; i < 12; ++i)
    {
        CreativeTabs *tab = CreativeTabs::creativeTabArray[i];
        if (tab != nullptr && renderCreativeInventoryHoveringText(tab, mouseX, mouseY))
            break;
    }

    if (destroyItemSlot != nullptr && selectedTabIndex == CreativeTabs::tabInventory->getTabIndex()
        && isPointInRegion(destroyItemSlot->xDisplayPosition, destroyItemSlot->yDisplayPosition, 16, 16, mouseX, mouseY))
    {
        drawCreativeTabHoveringText(StringTranslate::getInstance()->translateKey("inventory.binSlot"), mouseX, mouseY);
    }

    renderColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    renderDisable(RenderCapability::Lighting);
}

void GuiContainerCreative::drawGuiContainerForegroundLayer()
{
    if (selectedTabIndex >= 0 && selectedTabIndex < 12)
    {
        CreativeTabs *tab = CreativeTabs::creativeTabArray[selectedTabIndex];
        if (tab != nullptr && tab->drawInForegroundOfTab())
        {
            fontRenderer->drawString(tab->getTranslatedTabLabel(), 8, 6, 0x404040);
        }
    }
}

void GuiContainerCreative::drawGuiContainerBackgroundLayer(float_t)
{
    renderColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    RenderHelper::enableGUIStandardItemLighting();

    CreativeTabs *currentTab = (selectedTabIndex >= 0 && selectedTabIndex < 12)
        ? CreativeTabs::creativeTabArray[selectedTabIndex]
        : CreativeTabs::tabBlock;

    for (int i = 0; i < 12; ++i)
    {
        CreativeTabs *tab = CreativeTabs::creativeTabArray[i];
        if (tab != nullptr && tab->getTabIndex() != selectedTabIndex)
        {
            mc->renderEngine->bindTexture(mc->renderEngine->getTexture("/gui/allitems.png"));
            renderCreativeTab(tab);
        }
    }

    std::string bgPath = "/gui/creative_inv/" + currentTab->getBackgroundImageName();
    mc->renderEngine->bindTexture(mc->renderEngine->getTexture(bgPath.c_str()));
    drawTexturedModalRect(guiLeft, guiTop, 0, 0, xSize, ySize);

    if (searchField != nullptr && currentTab == CreativeTabs::tabAllSearch)
    {
        searchField->drawTextBox();
    }

    renderColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    int_t scrollX = guiLeft + 175;
    int_t scrollY = guiTop + 18;
    int_t scrollBottom = scrollY + 112;

    mc->renderEngine->bindTexture(mc->renderEngine->getTexture("/gui/allitems.png"));
    if (currentTab->shouldHidePlayerInventory())
    {
        int_t thumbOffset = (int_t)((float_t)(scrollBottom - scrollY - 17) * currentScroll);
        drawTexturedModalRect(scrollX, scrollY + thumbOffset, 232 + (needsScrollBars() ? 0 : 12), 0, 12, 15);
    }

    renderCreativeTab(currentTab);

    if (currentTab == CreativeTabs::tabInventory)
    {
        EntityPlayer *player = getContainerPlayer();
        if (player == nullptr && mc != nullptr) player = mc->thePlayer;
        drawPlayerOnGui(mc, guiLeft + 43, guiTop + 45, 20, (float_t)(guiLeft + 43) - xSize_lo, (float_t)(guiTop + 45 - 30) - ySize_lo, player);
    }

#if PLATFORM_PS2
    fontRenderer->drawStringWithShadow("L1", guiLeft - 13, guiTop - 14, 0xffe0e0e0);
    fontRenderer->drawStringWithShadow("R1", guiLeft + xSize + 2, guiTop - 14, 0xffe0e0e0);
#elif PLATFORM_XBOX
    drawControlIcon(mc, controlIconTexture(mc, "White"), guiLeft - 13, guiTop - 16);
    drawControlIcon(mc, controlIconTexture(mc, "Black"), guiLeft + xSize + 2, guiTop - 16);
#elif PLATFORM_WII
    fontRenderer->drawStringWithShadow("L", guiLeft - 10, guiTop - 14, 0xffe0e0e0);
    fontRenderer->drawStringWithShadow("R", guiLeft + xSize + 2, guiTop - 14, 0xffe0e0e0);
#endif
}

void GuiContainerCreative::renderCreativeTab(CreativeTabs *tab)
{
    if (tab == nullptr)
        return;

    bool isActive = (tab->getTabIndex() == selectedTabIndex);
    bool isFirstRow = tab->isTabInFirstRow();
    int_t col = tab->getTabColumn();
    int_t u = col * 28;
    int_t v = 0;
    int_t x = guiLeft + 28 * col;
    int_t y = guiTop;
    int_t w = 28;
    int_t h = 32;

    if (isActive)
        v += 32;

    if (col == 5)
        x = guiLeft + xSize - 28;
    else if (col > 0)
        x += col;

    if (isFirstRow)
        y -= 28;
    else
    {
        v += 64;
        y += ySize - 4;
    }

    renderDisable(RenderCapability::Lighting);
    drawTexturedModalRect(x, y, u, v, w, h);
    zLevel = 100.0f;
    itemRenderer->zLevel = 100.0f;
    x += 6;
    y += 8 + (isFirstRow ? 1 : -1);

    renderEnable(RenderCapability::Lighting);
    renderEnable(RenderCapability::RescaleNormal);

    int idx = tab->getTabIndex();
    if (idx >= 0 && idx < 12)
    {
        if (s_tabIcons[idx] == nullptr && tab->getTabIconItem() != nullptr)
        {
            s_tabIcons[idx] = new ItemStack(tab->getTabIconItem());
        }
        ItemStack *icon = s_tabIcons[idx];
        if (icon != nullptr)
        {
#if PLATFORM_GUI_FORCE_DEPTH_DISABLED
            renderEnable(RenderCapability::DepthTest);
            renderDepthMask(true);
            renderDepthFunc(RenderCompare::LessEqual);
#endif
            itemRenderer->renderItemIntoGUI(fontRenderer, mc->renderEngine, icon, x, y);
            itemRenderer->renderItemOverlayIntoGUI(fontRenderer, mc->renderEngine, icon, x, y);
#if PLATFORM_GUI_FORCE_DEPTH_DISABLED
            renderDisable(RenderCapability::DepthTest);
#endif
        }
    }

    renderDisable(RenderCapability::Lighting);
    itemRenderer->zLevel = 0.0f;
    zLevel = 0.0f;
}

void GuiContainerCreative::drawPlayerOnGui(Minecraft *mc, int_t x, int_t y, int_t scale, float_t mouseX, float_t mouseY, EntityPlayer *player)
{
    if (player == nullptr || mc == nullptr)
        return;

    renderEnable(RenderCapability::RescaleNormal);
    renderEnable(RenderCapability::ColorMaterial);
    renderClear(RenderClearMask::Depth);
    renderEnable(RenderCapability::DepthTest);
    renderPushMatrix();
    renderTranslate((float_t)x, (float_t)y, 50.0f);
    renderScale(-(float_t)scale, (float_t)scale, (float_t)scale);
    renderRotate(180.0f, 0.0f, 0.0f, 1.0f);

    float_t savedYawOffset = player->renderYawOffset;
    float_t savedYaw       = player->rotationYaw;
    float_t savedPitch     = player->rotationPitch;
    float_t f5 = (float_t)x - mouseX;
    float_t f6 = (float_t)(y - 30) - mouseY;

    renderRotate(135.0f, 0.0f, 1.0f, 0.0f);
    RenderHelper::enableStandardItemLighting();
    renderRotate(-135.0f, 0.0f, 1.0f, 0.0f);
    renderRotate(-(float_t)std::atan(f6 / 40.0f) * 20.0f, 1.0f, 0.0f, 0.0f);

    player->renderYawOffset = (float_t)std::atan(f5 / 40.0f) * 20.0f;
    player->rotationYaw     = (float_t)std::atan(f5 / 40.0f) * 40.0f;
    player->rotationPitch   = -(float_t)std::atan(f6 / 40.0f) * 20.0f;
    player->rotationYawHead = player->rotationYaw;
    player->entityBrightness = 1.0f;

    renderTranslate(0.0f, player->yOffset, 0.0f);
    RenderManager::instance->playerViewY = 180.0f;
    RenderManager::instance->renderEntityWithPosYaw(player, 0.0, 0.0, 0.0, 0.0f, 1.0f);

    player->entityBrightness = 0.0f;
    player->renderYawOffset  = savedYawOffset;
    player->rotationYaw      = savedYaw;
    player->rotationPitch    = savedPitch;

    renderPopMatrix();
    RenderHelper::disableStandardItemLighting();
    renderDisable(RenderCapability::RescaleNormal);
}

void GuiContainerCreative::actionPerformed(GuiButton *button)
{
    if (button->id == 0)
        mc->displayGuiScreen(new GuiAchievements(mc->statFileWriter));
    else if (button->id == 1)
        mc->displayGuiScreen(new GuiStats(this, mc->statFileWriter));
}
