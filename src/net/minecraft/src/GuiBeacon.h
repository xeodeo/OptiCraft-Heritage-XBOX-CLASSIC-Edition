#pragma once
#include "GuiContainer.h"
class InventoryPlayer; class TileEntityBeacon;
class GuiBeacon : public GuiContainer { public: GuiBeacon(InventoryPlayer *p, TileEntityBeacon *b, EntityPlayer *ep=nullptr); protected: void drawGuiContainerBackgroundLayer(float_t) override; void drawGuiContainerForegroundLayer() override; };
