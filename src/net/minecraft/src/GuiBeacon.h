#pragma once
#include "GuiContainer.h"
class InventoryPlayer; class TileEntityBeacon; class GuiButton;
class GuiBeacon : public GuiContainer { public: GuiBeacon(InventoryPlayer *p, TileEntityBeacon *b, EntityPlayer *ep=nullptr); void initGui() override; void updateScreen() override; protected: void actionPerformed(GuiButton*) override; void drawGuiContainerBackgroundLayer(float_t) override; void drawGuiContainerForegroundLayer() override; private: TileEntityBeacon *beacon; bool buttonsNotDrawn=true; };
