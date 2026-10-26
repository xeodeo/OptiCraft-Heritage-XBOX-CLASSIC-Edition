#pragma once

#include "GuiButton.h"

// net.minecraft.src.GuiButtonNextPage
class GuiButtonNextPage : public GuiButton
{
public:
    GuiButtonNextPage(int id, int x, int y, bool isNextPage);

    void drawButton(Minecraft *minecraft, int mouseX, int mouseY) override;

private:
    bool nextPage;
};
