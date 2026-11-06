#include "LegacyTradingScreen.h"

#include <algorithm>

#include "net/minecraft/src/EntityPlayer.h"
#include "net/minecraft/src/EntityPlayerSP.h"
#include "net/minecraft/src/FontRenderer.h"
#include "net/minecraft/src/IMerchant.h"
#include "net/minecraft/src/InventoryPlayer.h"
#include "net/minecraft/src/KeyBinding.h"
#include "net/minecraft/src/GameSettings.h"
#include "net/minecraft/src/ItemStack.h"
#include "net/minecraft/src/MerchantRecipe.h"
#include "net/minecraft/src/MerchantRecipeList.h"
#include "net/minecraft/src/Minecraft.h"
#include "net/minecraft/src/OpenGlHelper.h"
#include "net/minecraft/src/RenderEngine.h"
#include "net/minecraft/src/RenderHelper.h"
#include "net/minecraft/src/RenderItem.h"
#include "net/minecraft/src/SoundManager.h"
#include "net/minecraft/src/StatCollector.h"
#include "net/minecraft/src/ControlIcon.h"
#include "net/minecraft/src/UiStrings.h"
#include "platform/Input.h"
#include "platform/PlatformConfig.h"
#include "platform/RenderAPI.h"
#include "LegacyMenuHints.h"
#include "LegacySelectionCursor.h"
#include "pc/lwjgl/Keyboard.h"

RenderItem *LegacyTradingScreen::itemRenderer = new RenderItem();

namespace
{
// Geometry of LCE xuiscene_trading.xui (1280x720 scene, 588x360 group), halved
// into GUI units the way the console crafting screen is laid out.
constexpr int_t PANEL_W = 294;
constexpr int_t PANEL_H = 180;
constexpr int_t TITLE_Y = 12;
constexpr int_t BAR_X = 46;
constexpr int_t BAR_Y = 32;
constexpr int_t BAR_SLOT = 27;
constexpr int_t BAR_BOX = 26;
constexpr int_t LEFT_ARROW_X = 30;
constexpr int_t RIGHT_ARROW_X = 240;
constexpr int_t SECTION_Y0 = 74;
constexpr int_t SECTION_Y1 = 167;
constexpr int_t REQ_X0 = 12;
constexpr int_t REQ_X1 = 120;
constexpr int_t REQ_SLOT_X = 17;
constexpr int_t REQ1_Y = 100;
constexpr int_t REQ2_Y = 127;
constexpr int_t REQ_BOX = 21;
constexpr int_t REQ_NAME_X = 43;
constexpr int_t INV_X0 = 124;
constexpr int_t INV_X1 = 280;
constexpr int_t INV_GRID_X = 130;
constexpr int_t INV_GRID_Y = 90;
constexpr int_t INV_HOTBAR_Y = 146;
constexpr int_t INV_CELL = 16;

bool sameTradeItem(const ItemStack *have, const ItemStack *want)
{
    if (have == nullptr || want == nullptr || have->itemID != want->itemID)
        return false;
    ItemStack *w = const_cast<ItemStack *>(want);
    return !w->getHasSubtypes() || const_cast<ItemStack *>(have)->getItemDamage() == w->getItemDamage();
}
}

LegacyTradingScreen::LegacyTradingScreen(InventoryPlayer *playerInventory, IMerchant *merchantValue, World *worldValue,
                                         const std::string &title, EntityPlayer *playerValue)
    : inventory(playerInventory), merchant(merchantValue), world(worldValue), entityPlayer(playerValue),
      titleName(title), selectedSlot(0), offersStartIndex(0), hasUpdatedOnce(false),
      guiLeft(0), guiTop(0), xSize(PANEL_W), ySize(PANEL_H), ownerPlayerIndex(-1)
{
    if (titleName.empty() || titleName == "entity.Villager.name")
        titleName = StatCollector::translateToLocal("entity.Villager.name");
    if (playerValue != nullptr && mc != nullptr && playerValue == static_cast<EntityPlayer *>(mc->thePlayer2))
        ownerPlayerIndex = 1;
}

int LegacyTradingScreen::getOwnerPlayerIndex() const
{
    if (ownerPlayerIndex >= 0)
        return ownerPlayerIndex;
    if (entityPlayer != nullptr && mc != nullptr && entityPlayer == static_cast<EntityPlayer *>(mc->thePlayer2))
        return 1;
    if (mc != nullptr && mc->isScreenOwnedByPlayer2())
        return 1;
    return 0;
}

EntityPlayer *LegacyTradingScreen::player() const
{
    if (entityPlayer != nullptr)
        return entityPlayer;
    return mc != nullptr ? static_cast<EntityPlayer *>(mc->thePlayer) : nullptr;
}

void LegacyTradingScreen::initGui()
{
    GuiScreen::initGui();
    guiLeft = (width - xSize) / 2;
    guiTop = (height - ySize) / 2;
    refreshOffers();
}

void LegacyTradingScreen::onGuiClosed()
{
    GuiScreen::onGuiClosed();
    // ContainerMerchant.onCraftGuiClosed: the villager stops trading.
    if (merchant != nullptr)
        merchant->setCustomer(nullptr);
}

void LegacyTradingScreen::close()
{
    if (mc == nullptr)
        return;
    if (mc->isSplitScreenActive())
        mc->closePlayerScreen(getOwnerPlayerIndex());
    else
        mc->displayGuiScreen(nullptr);
}

void LegacyTradingScreen::refreshOffers()
{
    activeOffers.clear();
    MerchantRecipeList *offers = merchant != nullptr ? merchant->getRecipes(player()) : nullptr;
    if (offers == nullptr)
        return;
    for (int_t i = 0; i < offers->size(); ++i)
    {
        MerchantRecipe *recipe = offers->get(i);
        if (recipe != nullptr && !recipe->isRecipeDisabled())
            activeOffers.push_back(i);
    }
    const int_t count = static_cast<int_t>(activeOffers.size());
    if (offersStartIndex + DISPLAY_TRADES_COUNT > count)
        offersStartIndex = std::max<int_t>(0, count - DISPLAY_TRADES_COUNT);
    if (selectedSlot + offersStartIndex >= count)
        selectedSlot = std::max<int_t>(0, std::min<int_t>(DISPLAY_TRADES_COUNT - 1, count - 1 - offersStartIndex));
    hasUpdatedOnce = true;
}

MerchantRecipe *LegacyTradingScreen::offerAt(int_t activeIndex) const
{
    if (merchant == nullptr || activeIndex < 0 || activeIndex >= static_cast<int_t>(activeOffers.size()))
        return nullptr;
    MerchantRecipeList *offers = merchant->getRecipes(player());
    if (offers == nullptr)
        return nullptr;
    const int_t index = activeOffers[activeIndex];
    return index < offers->size() ? offers->get(index) : nullptr;
}

int_t LegacyTradingScreen::countMatches(const ItemStack *want) const
{
    if (want == nullptr || inventory == nullptr || inventory->mainInventory == nullptr)
        return 0;
    int_t total = 0;
    for (int_t i = 0; i < 36; ++i)
    {
        ItemStack *st = inventory->mainInventory[i];
        if (st != nullptr && st->isValid() && sameTradeItem(st, want))
            total += st->stackSize;
    }
    return total;
}

void LegacyTradingScreen::removeResources(const ItemStack *want)
{
    if (want == nullptr || inventory == nullptr || inventory->mainInventory == nullptr)
        return;
    int_t toTake = want->stackSize;
    for (int_t i = 0; i < 36 && toTake > 0; ++i)
    {
        ItemStack *st = inventory->mainInventory[i];
        if (st == nullptr || !st->isValid() || !sameTradeItem(st, want))
            continue;
        const int_t take = std::min(st->stackSize, toTake);
        st->stackSize -= take;
        toTake -= take;
        if (st->stackSize <= 0)
        {
            delete st;
            inventory->mainInventory[i] = nullptr;
        }
    }
    inventory->inventoryChanged = true;
}

bool LegacyTradingScreen::canMake(MerchantRecipe *recipe) const
{
    if (recipe == nullptr || recipe->isRecipeDisabled())
        return false;
    ItemStack *buyA = recipe->getItemToBuy();
    ItemStack *buyB = recipe->getSecondItemToBuy();
    if (buyA == nullptr || countMatches(buyA) < buyA->stackSize)
        return false;
    return buyB == nullptr || countMatches(buyB) >= buyB->stackSize;
}

void LegacyTradingScreen::moveSelection(int dir)
{
    const int_t count = static_cast<int_t>(activeOffers.size());
    bool changed = false;
    if (dir < 0)
    {
        if (selectedSlot == 0)
        {
            if (offersStartIndex > 0) { --offersStartIndex; changed = true; }
        }
        else { --selectedSlot; changed = true; }
    }
    else
    {
        if (selectedSlot == DISPLAY_TRADES_COUNT - 1)
        {
            if (offersStartIndex + DISPLAY_TRADES_COUNT < count) { ++offersStartIndex; changed = true; }
        }
        else if (selectedSlot + offersStartIndex + 1 < count) { ++selectedSlot; changed = true; }
    }
    if (changed && mc != nullptr && mc->sndManager != nullptr)
        mc->sndManager->playSoundFX("random.focus", 1.0f, 1.0f);
}

void LegacyTradingScreen::trade()
{
    MerchantRecipe *recipe = offerAt(selectedSlot + offersStartIndex);
    EntityPlayer *p = player();
    if (recipe == nullptr || p == nullptr || !canMake(recipe))
    {
        if (mc != nullptr && mc->sndManager != nullptr)
            mc->sndManager->playSoundFX("note.bass", 1.0f, 0.8f);
        return;
    }

    // Copy everything first: useRecipe may replace this recipe in the villager's list.
    ItemStack *buyA = recipe->getItemToBuy() ? recipe->getItemToBuy()->copy() : nullptr;
    ItemStack *buyB = recipe->getSecondItemToBuy() ? recipe->getSecondItemToBuy()->copy() : nullptr;
    ItemStack *result = recipe->getItemToSell()->copy();

    removeResources(buyA);
    removeResources(buyB);
    delete buyA;
    delete buyB;

    // SlotMerchantResult.onPickupFromSlot: crafting stats, then the villager uses the recipe.
    result->onCrafting(p->worldObj, p, result->stackSize);
    merchant->useRecipe(recipe);

    if (!inventory->addItemStackToInventory(result) || result->stackSize > 0)
    {
        if (result->stackSize > 0)
            p->dropPlayerItem(result);
        else
            delete result;
    }
    else
    {
        delete result;
    }
    inventory->inventoryChanged = true;

    if (mc != nullptr && mc->sndManager != nullptr)
        mc->sndManager->playSoundFX("random.pop", 1.0f, 1.0f);
    refreshOffers();
}

void LegacyTradingScreen::updateScreen()
{
    GuiScreen::updateScreen();

    // Same rule as GuiMerchant: the trade ends when the villager drops its customer.
    if (merchant == nullptr || merchant->getCustomer() != player())
    {
        close();
        return;
    }
    refreshOffers();

    const PlatformTextInputSnapshot pad = platformTextInputSnapshot(platformMenuPad());
    if (!pad.connected)
        return;
    if (pad.pressed & PLATFORM_TEXT_LEFT)  moveSelection(-1);
    if (pad.pressed & PLATFORM_TEXT_RIGHT) moveSelection(1);
    if (pad.pressed & PLATFORM_TEXT_TYPE)  trade();
    if (pad.pressed & (PLATFORM_TEXT_CLOSE | PLATFORM_TEXT_SHIFT))
    {
        if (mc != nullptr && mc->sndManager != nullptr)
            mc->sndManager->playSoundFX("random.back", 1.0f, 1.0f);
        close();
    }
}

void LegacyTradingScreen::keyTyped(char_t c, int_t key)
{
    const bool inventoryKey = mc != nullptr && mc->gameSettings != nullptr &&
        mc->gameSettings->keyBindInventory != nullptr && key == mc->gameSettings->keyBindInventory->keyCode;
    if (key == lwjgl::Keyboard::KEY_ESCAPE || inventoryKey)
    {
        if (mc != nullptr && mc->sndManager != nullptr)
            mc->sndManager->playSoundFX("random.back", 1.0f, 1.0f);
        close();
        return;
    }
    if (key == lwjgl::Keyboard::KEY_LEFT || key == lwjgl::Keyboard::KEY_A) { moveSelection(-1); return; }
    if (key == lwjgl::Keyboard::KEY_RIGHT || key == lwjgl::Keyboard::KEY_D) { moveSelection(1); return; }
    if (key == lwjgl::Keyboard::KEY_RETURN || key == lwjgl::Keyboard::KEY_SPACE) { trade(); return; }
    GuiScreen::keyTyped(c, key);
}

void LegacyTradingScreen::mouseClicked(int_t mouseX, int_t mouseY, int_t button)
{
    GuiScreen::mouseClicked(mouseX, mouseY, button);
    const int_t count = static_cast<int_t>(activeOffers.size());
    for (int_t i = 0; i < DISPLAY_TRADES_COUNT && i + offersStartIndex < count; ++i)
    {
        const int_t sx = guiLeft + BAR_X + i * BAR_SLOT;
        const int_t sy = guiTop + BAR_Y;
        if (mouseX >= sx && mouseX < sx + BAR_BOX && mouseY >= sy && mouseY < sy + BAR_BOX)
        {
            if (selectedSlot != i && mc != nullptr && mc->sndManager != nullptr)
                mc->sndManager->playSoundFX("random.focus", 1.0f, 1.0f);
            selectedSlot = i;
            return;
        }
    }
}

void LegacyTradingScreen::drawSection(int_t x0, int_t y0, int_t x1, int_t y1)
{
    // XuiSkinSelectSectionBackground: a sunken darker panel inside the main one.
    drawRect(x0, y0, x1, y1, 0xffa8a8a8);
    drawRect(x0, y0, x1, y0 + 1, 0xff6a6a6a);
    drawRect(x0, y0, x0 + 1, y1, 0xff6a6a6a);
    drawRect(x0 + 1, y1 - 1, x1, y1, 0xffe8e8e8);
    drawRect(x1 - 1, y0 + 1, x1, y1, 0xffe8e8e8);
}

void LegacyTradingScreen::drawSlotBox(int_t x, int_t y, int_t size, bool red)
{
    drawRect(x, y, x + size, y + size, 0xff8b8b8b);
    drawRect(x, y, x + size, y + 1, 0xff373737);
    drawRect(x, y, x + 1, y + size, 0xff373737);
    drawRect(x + size - 1, y + 1, x + size, y + size, 0xffffffff);
    drawRect(x + 1, y + size - 1, x + size, y + size, 0xffffffff);
    if (red)
        drawRect(x + 1, y + 1, x + size - 1, y + size - 1, 0x80cc2020);
}

void LegacyTradingScreen::drawItem(ItemStack *stack, int_t x, int_t y)
{
    if (stack == nullptr)
        return;
    renderEnable(RenderCapability::Lighting);
    itemRenderer->renderItemIntoGUI(fontRenderer, mc->renderEngine, stack, x, y);
    itemRenderer->renderItemOverlayIntoGUI(fontRenderer, mc->renderEngine, stack, x, y);
    renderDisable(RenderCapability::Lighting);
}

void LegacyTradingScreen::drawTooltip(ItemStack *stack, int_t x, int_t y)
{
    if (stack == nullptr || fontRenderer == nullptr)
        return;
    std::vector<std::string> lines = stack->getItemNameandInformation();
    if (lines.empty())
        return;
    int_t w = 0;
    for (const std::string &l : lines)
        w = std::max(w, fontRenderer->getStringWidth(l));
    if (x + w + 6 > width)
        x = width - w - 6;
    if (x < 4)
        x = 4;
    const int_t h = 8 + (lines.size() > 1 ? 2 + (static_cast<int_t>(lines.size()) - 1) * 10 : 0);
    drawRect(x - 3, y - 3, x + w + 3, y + h + 3, static_cast<int_t>(0xf0100010u));
    drawRect(x - 3, y - 3, x + w + 3, y - 2, 0x505000ff);
    drawRect(x - 3, y + h + 2, x + w + 3, y + h + 3, 0x5028007f);
    for (std::size_t i = 0; i < lines.size(); ++i)
    {
        fontRenderer->drawStringWithShadow(lines[i], x, y, i == 0 ? 0xffffffff : 0xffaaaaaa);
        y += (i == 0) ? 12 : 10;
    }
}

void LegacyTradingScreen::drawScreen(int_t mouseX, int_t mouseY, float_t partialTick)
{
    (void)partialTick;
    drawDefaultBackground();
    if (!hasUpdatedOnce)
        refreshOffers();

    const int_t left = guiLeft;
    const int_t top = guiTop;
    const int_t right = guiLeft + xSize;
    const int_t bottom = guiTop + ySize;

    // Main panel, same bevel as the console crafting screen.
    drawRect(left + 2, top + 2, right + 2, bottom + 2, 0x68000000);
    drawRect(left - 1, top - 1, right + 1, bottom + 1, 0xff343434);
    drawRect(left, top, right, bottom, 0xffc6c6c6);
    drawRect(left + 2, top + 1, right - 2, top + 2, 0xffededed);
    drawRect(left + 1, top + 2, left + 2, bottom - 2, 0xffededed);
    drawRect(left + 2, bottom - 2, right - 2, bottom - 1, 0xff858585);
    drawRect(right - 2, top + 2, right - 1, bottom - 2, 0xff858585);

    RenderHelper::enableGUIStandardItemLighting();
    renderPushMatrix();
    renderColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    renderEnable(RenderCapability::RescaleNormal);
    OpenGlHelper::setLightmapTextureCoords(OpenGlHelper::lightmapTexUnit, 240.0f, 240.0f);
    renderDisable(RenderCapability::Lighting);

    const int_t count = static_cast<int_t>(activeOffers.size());
    MerchantRecipe *selected = offerAt(selectedSlot + offersStartIndex);
    ItemStack *hovered = nullptr;

    // Villager name; the offered item is named in the description under the selected offer.
    const std::string &title = titleName;
    fontRenderer->drawString(title, left + (xSize - fontRenderer->getStringWidth(title)) / 2, top + TITLE_Y, 0x404040);

    // Offer bar.
    for (int_t i = 0; i < DISPLAY_TRADES_COUNT; ++i)
    {
        const int_t sx = left + BAR_X + i * BAR_SLOT;
        const int_t sy = top + BAR_Y;
        MerchantRecipe *recipe = offerAt(i + offersStartIndex);
        drawSlotBox(sx, sy, BAR_BOX, recipe != nullptr && !canMake(recipe));
        if (recipe != nullptr)
        {
            drawItem(recipe->getItemToSell(), sx + 5, sy + 5);
            if (mouseX >= sx && mouseX < sx + BAR_BOX && mouseY >= sy && mouseY < sy + BAR_BOX && i != selectedSlot)
                hovered = recipe->getItemToSell();
        }
    }
    if (offersStartIndex > 0)
        fontRenderer->drawStringWithShadow("<", left + LEFT_ARROW_X + 6, top + BAR_Y + 9, 0xffffffff);
    if (offersStartIndex + DISPLAY_TRADES_COUNT < count)
        fontRenderer->drawStringWithShadow(">", left + RIGHT_ARROW_X + 6, top + BAR_Y + 9, 0xffffffff);

    // Required items.
    drawSection(left + REQ_X0, top + SECTION_Y0, left + REQ_X1, top + SECTION_Y1);
    const std::string required = uiText("Required items for trade");
    const int_t reqW = REQ_X1 - REQ_X0 - 6;
    const int_t reqLines = fontRenderer->splitStringWidth(required, reqW) / 8;
    const int_t reqTextW = reqLines > 1 ? reqW : fontRenderer->getStringWidth(required);
    fontRenderer->drawSplitString(required, left + REQ_X0 + (REQ_X1 - REQ_X0 - reqTextW) / 2, top + SECTION_Y0 + 4, reqW, 0x404040);

    ItemStack *buyA = selected != nullptr ? selected->getItemToBuy() : nullptr;
    ItemStack *buyB = selected != nullptr ? selected->getSecondItemToBuy() : nullptr;
    const int_t reqY[2] = {REQ1_Y, REQ2_Y};
    ItemStack *reqItems[2] = {buyA, buyB};
    for (int r = 0; r < 2; ++r)
    {
        const int_t sx = left + REQ_SLOT_X;
        const int_t sy = top + reqY[r];
        ItemStack *want = reqItems[r];
        drawSlotBox(sx, sy, REQ_BOX, want != nullptr && countMatches(want) < want->stackSize);
        if (want == nullptr)
            continue;
        drawItem(want, sx + 3, sy + 3);
        const std::string name = want->getDisplayName();
        const int_t nameW = REQ_X1 - REQ_NAME_X - 4;
        const int_t lines = std::max<int_t>(1, fontRenderer->splitStringWidth(name, nameW) / 8);
        fontRenderer->drawSplitString(name, left + REQ_NAME_X, sy + (REQ_BOX - lines * 9) / 2 + 1, nameW, 0x404040);
        if (mouseX >= sx && mouseX < sx + REQ_BOX && mouseY >= sy && mouseY < sy + REQ_BOX)
            hovered = want;
    }

    // Inventory (display only, as on the console).
    drawSection(left + INV_X0, top + SECTION_Y0, left + INV_X1, top + SECTION_Y1);
    const std::string invLabel = uiText("Inventory");
    fontRenderer->drawString(invLabel, left + INV_X0 + (INV_X1 - INV_X0 - fontRenderer->getStringWidth(invLabel)) / 2,
                             top + SECTION_Y0 + 4, 0x404040);
    for (int_t r = 0; r < 4; ++r)
    {
        for (int_t c = 0; c < 9; ++c)
        {
            const int_t slotIndex = r < 3 ? 9 + r * 9 + c : c;
            const int_t sx = left + INV_GRID_X + c * INV_CELL;
            const int_t sy = top + (r < 3 ? INV_GRID_Y + r * INV_CELL : INV_HOTBAR_Y);
            drawSlotBox(sx, sy, INV_CELL, false);
            ItemStack *st = (inventory != nullptr && inventory->mainInventory != nullptr) ? inventory->mainInventory[slotIndex] : nullptr;
            if (st != nullptr && st->isValid())
            {
                drawItem(st, sx, sy);
                if (mouseX >= sx && mouseX < sx + INV_CELL && mouseY >= sy && mouseY < sy + INV_CELL)
                    hovered = st;
            }
        }
    }

    // Selector over the chosen offer (CraftingPanelHighlight).
    if (count > 0)
    {
        const int_t cx = left + BAR_X + selectedSlot * BAR_SLOT + BAR_BOX / 2;
        legacyDrawSelectionCursorCentered(mc, cx, top + BAR_Y + BAR_BOX / 2, 34, zLevel + 64.0f);
    }

    renderDisable(RenderCapability::RescaleNormal);
    RenderHelper::disableStandardItemLighting();
    renderPopMatrix();

    // Offer description under the selected slot (HtmlTextPanel), or the hovered item.
    renderDisable(RenderCapability::DepthTest);
    if (hovered != nullptr)
        drawTooltip(hovered, mouseX + 12, mouseY - 12);
    else if (selected != nullptr)
        drawTooltip(selected->getItemToSell(), left + BAR_X + selectedSlot * BAR_SLOT, top + BAR_Y + BAR_BOX + 6);
    renderEnable(RenderCapability::DepthTest);

#if PLATFORM_PS2
    static const std::string buttons[] = {"D-Pad", "Cross", "Circle"};
#elif PLATFORM_XBOX || PLATFORM_WII
    static const std::string buttons[] = {"D-Pad", "A", "B"};
#else
    static const std::string buttons[] = {"Arrows", "Enter", "Esc"};
#endif
    const std::string actions[] = {uiText("Navigate"), uiText("Trade"), uiText("Back")};
    drawControlHintRow(mc, width, legacyHintRowY(height), buttons, actions, 3);
}
