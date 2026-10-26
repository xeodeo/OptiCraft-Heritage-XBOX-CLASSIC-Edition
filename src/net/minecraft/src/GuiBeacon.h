#pragma once

#include <vector>
#include <string>
#include "GuiContainer.h"
#include "GuiButton.h"

class TileEntityBeacon;
class InventoryPlayer;
class EntityPlayer;
class GuiBeacon;

class GuiBeaconButton : public GuiButton
{
public:
    GuiBeaconButton(int id, int x, int y, const std::string &texture, int iconX, int iconY);

    void drawButton(Minecraft *mc, int_t mouseX, int_t mouseY) override;
    bool func_82255_b() const { return field_82256_n; }
    void func_82254_b(bool flag) { field_82256_n = flag; }
    virtual void func_82251_b(int x, int y);

protected:
    std::string buttonTexture;
    int field_82257_l = 0;
    int field_82258_m = 0;
    bool field_82256_n = false;
};

class GuiBeaconButtonPower : public GuiBeaconButton
{
public:
    GuiBeaconButtonPower(GuiBeacon *gui, int id, int x, int y, int effectId, int tier);

    void func_82251_b(int x, int y) override;

    int getEffectId() const { return field_82261_l; }
    int getTier() const { return field_82262_m; }

private:
    GuiBeacon *beaconGui = nullptr;
    int field_82261_l = 0;
    int field_82262_m = 0;
};

class GuiBeaconButtonConfirm : public GuiBeaconButton
{
public:
    GuiBeaconButtonConfirm(GuiBeacon *gui, int id, int x, int y);

    void func_82251_b(int x, int y) override;

private:
    GuiBeacon *beaconGui = nullptr;
};

class GuiBeaconButtonCancel : public GuiBeaconButton
{
public:
    GuiBeaconButtonCancel(GuiBeacon *gui, int id, int x, int y);

    void func_82251_b(int x, int y) override;

private:
    GuiBeacon *beaconGui = nullptr;
};

class GuiBeacon : public GuiContainer
{
public:
    GuiBeacon(InventoryPlayer *playerInv, TileEntityBeacon *beacon, EntityPlayer *player);
    ~GuiBeacon() override = default;

    void initGui() override;
    void updateScreen() override;

    void drawCreativeTabHoveringText(const std::string &text, int x, int y);

protected:
    void actionPerformed(GuiButton *button) override;
    void drawGuiContainerForegroundLayer() override;
    void drawGuiContainerBackgroundLayer(float_t partialTick) override;

private:
    TileEntityBeacon *beacon = nullptr;
    GuiBeaconButtonConfirm *beaconConfirmButton = nullptr;
    bool buttonsNotDrawn = true;
};
