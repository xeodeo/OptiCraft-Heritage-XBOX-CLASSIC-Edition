#pragma once
#include "GuiContainer.h"
class InventoryPlayer; class World; class GuiTextField;
class GuiRepair : public GuiContainer { public: GuiRepair(InventoryPlayer *p, World *w, int x, int y, int z, EntityPlayer *ep=nullptr); ~GuiRepair() override; void initGui() override; void onGuiClosed() override; void drawScreen(int_t,int_t,float_t) override; protected: void keyTyped(char_t,int_t) override; void mouseClicked(int_t,int_t,int_t) override; void drawGuiContainerBackgroundLayer(float_t) override; void drawGuiContainerForegroundLayer() override; private: GuiTextField *itemNameField=nullptr; };
