#pragma once

#include "GuiContainer.h"
#include <string>
#include <vector>

class CreativeCrafting;
class CreativeTabs;
class EntityPlayer;
class GuiTextField;
class InventoryBasic;
class ItemStack;
class RenderItem;
class Slot;

// net.minecraft.src.GuiContainerCreative
class GuiContainerCreative : public GuiContainer
{
public:
    explicit GuiContainerCreative(EntityPlayer *player);
    ~GuiContainerCreative() override;

    void updateScreen() override;
    void initGui() override;
    void onGuiClosed() override;
    void handleMouseInput() override;
    void mouseClicked(int_t mouseX, int_t mouseY, int_t button) override;
    void mouseMovedOrUp(int_t mouseX, int_t mouseY, int_t button) override;
    void keyTyped(char_t c, int_t key) override;
    void drawScreen(int_t mouseX, int_t mouseY, float_t partialTick) override;
    bool holdRepeatTakeOne(Slot *slot) override;
    void returnInitialStack(Slot *slot) override;
    Slot *getControllerNavigationTarget(Slot *selected, int_t dirX, int_t dirY) override;
    bool scrollRows(int_t direction);

    static InventoryBasic *getInventory();
    int func_74230_h() const { return selectedTabIndex; }
    void setCurrentCreativeTab(CreativeTabs *tab);

protected:
    void handleMouseClick(Slot *slot, int_t slotId, int_t button, bool shift) override;
    void drawGuiContainerForegroundLayer() override;
    void drawGuiContainerBackgroundLayer(float_t partialTick) override;
    void actionPerformed(GuiButton *button) override;

    bool func_74232_a(CreativeTabs *tab, int_t mouseX, int_t mouseY);
    bool renderCreativeInventoryHoveringText(CreativeTabs *tab, int_t mouseX, int_t mouseY);
    void renderCreativeTab(CreativeTabs *tab);
    void drawCreativeTabHoveringText(const std::string &text, int_t x, int_t y);
    bool isPointInRegion(int_t x, int_t y, int_t w, int_t h, int_t pointX, int_t pointY);
    bool needsScrollBars();
    void updateCreativeSearch();

    static void drawPlayerOnGui(Minecraft *mc, int_t x, int_t y, int_t scale, float_t mouseX, float_t mouseY, EntityPlayer *player);

private:
    static InventoryBasic inventory;
    static int selectedTabIndex;
    float_t currentScroll;
    bool isScrolling;
    bool wasClicking;
    GuiTextField *searchField;
    std::vector<Slot *> backupContainerSlots;
    Slot *destroyItemSlot;
    bool field_74234_w;
    CreativeCrafting *creativeCrafting;
    float_t xSize_lo;
    float_t ySize_lo;

    static ItemStack *s_tabIcons[12];
    static RenderItem *creativeItemRenderer;
};
