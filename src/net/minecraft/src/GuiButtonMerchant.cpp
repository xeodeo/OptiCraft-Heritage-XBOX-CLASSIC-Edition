#include "GuiButtonMerchant.h"

#include "Minecraft.h"
#include "RenderEngine.h"
#include "platform/RenderAPI.h"

GuiButtonMerchant::GuiButtonMerchant(int_t id, int_t x, int_t y, bool mirrored)
	: GuiButton(id, x, y, 12, 19, ""), mirrored(mirrored)
{
}

void GuiButtonMerchant::drawButton(Minecraft *mc, int_t mouseX, int_t mouseY)
{
	if (!enabled2 || mc == nullptr || mc->renderEngine == nullptr)
		return;

	int_t texture = mc->renderEngine->getTexture("/gui/trading.png");
	renderColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	mc->renderEngine->bindTexture(texture);

	bool hovered = mouseX >= xPosition && mouseY >= yPosition && mouseX < xPosition + width && mouseY < yPosition + height;
	int_t u = 176;
	int_t v = 0;

	if (!enabled)
	{
		u += width * 2;
	}
	else if (hovered)
	{
		u += width;
	}

	if (!mirrored)
	{
		v += height;
	}

	drawTexturedModalRect(xPosition, yPosition, u, v, width, height);
}
