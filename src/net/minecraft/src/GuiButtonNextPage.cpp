#include "GuiButtonNextPage.h"
#include "Minecraft.h"
#include "RenderEngine.h"
#include "platform/RenderAPI.h"

GuiButtonNextPage::GuiButtonNextPage(int id, int x, int y, bool isNextPage)
    : GuiButton(id, x, y, 23, 13, "")
    , nextPage(isNextPage)
{
}

void GuiButtonNextPage::drawButton(Minecraft *minecraft, int mouseX, int mouseY)
{
    if (enabled2)
    {
        bool hovered = mouseX >= xPosition && mouseY >= yPosition && mouseX < xPosition + width && mouseY < yPosition + height;
        renderColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        minecraft->renderEngine->bindTexture(minecraft->renderEngine->getTexture("/gui/book.png"));
        int u = 0;
        int v = 192;
        if (hovered)
        {
            u += 23;
        }
        if (!nextPage)
        {
            v += 13;
        }
        drawTexturedModalRect(xPosition, yPosition, u, v, 23, 13);
    }
}
