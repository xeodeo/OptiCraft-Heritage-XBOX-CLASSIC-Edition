#pragma once

#include <string>
#include <vector>

#include "net/minecraft/src/GuiScreen.h"

class InventoryPlayer;
class World;
class EntityPlayer;
class RenderItem;
class ItemStack;
class IMerchant;
class MerchantRecipe;

// Console trading menu (LCE UIScene_TradingMenu / IUIScene_TradingMenu): a row of
// offers picked with the D-pad, the items the selected offer costs, and the
// player's inventory. A trades straight from the inventory, as on the 360.
class LegacyTradingScreen : public GuiScreen
{
public:
    LegacyTradingScreen(InventoryPlayer *playerInventory, IMerchant *merchant, World *world,
                        const std::string &title, EntityPlayer *player);

    void initGui() override;
    void drawScreen(int_t mouseX, int_t mouseY, float_t partialTick) override;
    void updateScreen() override;
    void onGuiClosed() override;
    bool doesGuiPauseGame() override { return false; }
    int getOwnerPlayerIndex() const override;

    IMerchant *getMerchant() const { return merchant; }
    void detachMerchant() { merchant = nullptr; }

protected:
    bool usesSpecializedMenuNavigation() const override { return true; }
    void keyTyped(char_t c, int_t key) override;
    void mouseClicked(int_t mouseX, int_t mouseY, int_t button) override;

private:
    static const int DISPLAY_TRADES_COUNT = 7;

    EntityPlayer *player() const;
    void refreshOffers();
    MerchantRecipe *offerAt(int_t activeIndex) const;
    void moveSelection(int dir);
    void trade();
    bool canMake(MerchantRecipe *recipe) const;
    int_t countMatches(const ItemStack *want) const;
    void removeResources(const ItemStack *want);
    void close();
    void drawSection(int_t x0, int_t y0, int_t x1, int_t y1);
    void drawSlotBox(int_t x, int_t y, int_t size, bool red);
    void drawItem(ItemStack *stack, int_t x, int_t y);
    void drawTooltip(ItemStack *stack, int_t x, int_t y);

    InventoryPlayer *inventory;
    IMerchant *merchant;
    World *world;
    EntityPlayer *entityPlayer;
    std::string titleName;

    std::vector<int_t> activeOffers;
    int_t selectedSlot;
    int_t offersStartIndex;
    bool hasUpdatedOnce;

    int_t guiLeft;
    int_t guiTop;
    int_t xSize;
    int_t ySize;
    int ownerPlayerIndex;

    static RenderItem *itemRenderer;
};
