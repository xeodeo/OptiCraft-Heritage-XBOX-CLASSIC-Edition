#pragma once

#include "GuiScreen.h"

class GuiButton;
class GuiTextField;
class TileEntityCommandBlock;

// net.minecraft.src.GuiCommandBlock (Java 1.5.2 + Legacy Console UI support)
class GuiCommandBlock : public GuiScreen
{
public:
	explicit GuiCommandBlock(TileEntityCommandBlock *tile);
	~GuiCommandBlock() override;

	void initGui() override;
	void onGuiClosed() override;
	void updateScreen() override;
	void drawScreen(int_t mouseX, int_t mouseY, float_t partialTick) override;

protected:
	void actionPerformed(GuiButton *button) override;
	void keyTyped(char_t c, int_t key) override;
	void mouseClicked(int_t mouseX, int_t mouseY, int_t button) override;

private:
	GuiTextField *commandTextField;
	TileEntityCommandBlock *commandBlock;
	GuiButton *doneBtn;
	GuiButton *cancelBtn;
};
