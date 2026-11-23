#include "TileEntitySpecialRenderer.h"
#include "TileEntityRenderer.h"
#include "FontRenderer.h"
#include "RenderEngine.h"
#include "Minecraft.h"

TileEntitySpecialRenderer::TileEntitySpecialRenderer() {
    tileEntityRenderer = nullptr;
}

void TileEntitySpecialRenderer::setTileEntityRenderer(TileEntityRenderer* renderer) {
    tileEntityRenderer = renderer;
}

FontRenderer* TileEntitySpecialRenderer::getFontRenderer() {
    if (tileEntityRenderer != nullptr && tileEntityRenderer->fontRenderer != nullptr)
        return tileEntityRenderer->fontRenderer;
    Minecraft *mc = Minecraft::getMinecraft();
    return mc != nullptr ? mc->fontRenderer : nullptr;
}

void TileEntitySpecialRenderer::bindTextureByName(const std::string& path) {
    RenderEngine* renderengine = tileEntityRenderer != nullptr ? tileEntityRenderer->renderEngine : nullptr;
    if (renderengine == nullptr) {
        Minecraft *mc = Minecraft::getMinecraft();
        if (mc != nullptr)
            renderengine = mc->renderEngine;
    }
    if (renderengine != nullptr)
        renderengine->bindTexture(renderengine->getTexture(path));
}
