#pragma once

#include "GuiButton.h"

// net.minecraft.src.GuiButtonMerchant
class GuiButtonMerchant : public GuiButton
{
public:
	GuiButtonMerchant(int_t id, int_t x, int_t y, bool mirrored);

	void drawButton(Minecraft *mc, int_t mouseX, int_t mouseY) override;

	bool isMirrored() const { return mirrored; }

private:
	bool mirrored;
};
