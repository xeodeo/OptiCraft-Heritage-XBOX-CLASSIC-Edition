#pragma once

#include <memory>
#include "GuiScreen.h"

class GuiButton;
class ServerNBTStorage;

// Server Information & Player List card owned by ServerUtilsMod (Java + Legacy UI + Xbox pad).
class GuiServerInfo : public GuiScreen
{
public:
    GuiServerInfo(GuiScreen *parent, const std::shared_ptr<ServerNBTStorage> &server);

    void initGui() override;
    void drawScreen(int_t mouseX, int_t mouseY, float_t partialTick) override;
    void handleSpecializedMenuInput() override;
    bool usesSpecializedMenuNavigation() const override;

protected:
    void actionPerformed(GuiButton *button) override;
    void keyTyped(char_t c, int_t key) override;

private:
    void drawLegacyScreen(float_t partialTick);
    GuiScreen *parentScreen;
    std::shared_ptr<ServerNBTStorage> server;
    bool openLatch = true;
};
