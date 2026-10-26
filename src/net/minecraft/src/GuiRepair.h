#pragma once
#include "GuiContainer.h"
class InventoryPlayer; class World;
class GuiRepair : public GuiContainer { public: GuiRepair(InventoryPlayer *p, World *w, int x, int y, int z, EntityPlayer *ep=nullptr); protected: void drawGuiContainerBackgroundLayer(float_t) override; void drawGuiContainerForegroundLayer() override; };
