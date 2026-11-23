#include "TileEntitySignRenderer.h"

#include "platform/RenderAPI.h"
#include "TileEntitySign.h"
#include "SignModel.h"
#include "Block.h"
#include "FontRenderer.h"
#include "ModelRenderer.h"

TileEntitySignRenderer::TileEntitySignRenderer()
    : signModel(new SignModel())
{
}

TileEntitySignRenderer::~TileEntitySignRenderer()
{
    delete signModel;
    signModel = nullptr;
}

bool TileEntitySignRenderer::renderTileEntitySignParts(TileEntitySign *tileentitysign, double d, double d1, double d2,
    bool renderStaticPart, bool renderDynamicPart)
{
    if (tileentitysign == nullptr)
        return false;

    Block *block = tileentitysign->getBlockType();
    renderPushMatrix();
    const float f1 = 0.6666667f;
    if (block == Block::signPost)
    {
        renderTranslate((float)d + 0.5f, (float)d1 + 0.75f * f1, (float)d2 + 0.5f);
        const float f2 = (float)(tileentitysign->getBlockMetadata() * 360) / 16.0f;
        renderRotate(-f2, 0.0f, 1.0f, 0.0f);
        signModel->signStick->showModel = true;
    }
    else
    {
        const int i = tileentitysign->getBlockMetadata();
        float f3 = 0.0f;
        if (i == 2) f3 = 180.0f;
        if (i == 4) f3 = 90.0f;
        if (i == 5) f3 = -90.0f;
        renderTranslate((float)d + 0.5f, (float)d1 + 0.75f * f1, (float)d2 + 0.5f);
        renderRotate(-f3, 0.0f, 1.0f, 0.0f);
        renderTranslate(0.0f, -0.3125f, -0.4375f);
        signModel->signStick->showModel = false;
    }

    if (renderStaticPart)
    {
        bindTextureByName("/item/sign.png");
        renderPushMatrix();
        renderScale(f1, -f1, -f1);
        signModel->renderStaticPart();
        renderPopMatrix();
    }

    if (renderDynamicPart)
    {
        FontRenderer *fontrenderer = getFontRenderer();
        if (fontrenderer != nullptr)
        {
            const float f4 = (1.0f / 60.0f) * f1;
            renderTranslate(0.0f, 0.5f * f1, 0.07f * f1);
            renderScale(f4, -f4, f4);
            renderNormal3f(0.0f, 0.0f, -1.0f * f4);
            renderDepthMask(false);
            for (int k = 0; k < 4; k++)
            {
                std::string s = tileentitysign->signText[k];
                if (k == tileentitysign->lineBeingEdited)
                    s = "> " + s + "\xc2\xa7r <";
                fontrenderer->drawString(s, -fontrenderer->getStringWidth(s) / 2, k * 10 - 20, 0);
            }
            renderDepthMask(true);
        }
    }

    renderColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    renderPopMatrix();
    return true;
}

void TileEntitySignRenderer::renderTileEntitySignAt(TileEntitySign *tileentitysign, double d, double d1, double d2, float f)
{
    (void)f;
    (void)renderTileEntitySignParts(tileentitysign, d, d1, d2, true, true);
}

bool TileEntitySignRenderer::renderStaticPart(TileEntitySign *tileentitysign, double d, double d1, double d2)
{
    return renderTileEntitySignParts(tileentitysign, d, d1, d2, true, false);
}

void TileEntitySignRenderer::prepareStaticPart()
{
    if (signModel != nullptr)
        signModel->prepareStaticPart();
}

bool TileEntitySignRenderer::renderDynamicPart(TileEntitySign *tileentitysign, double d, double d1, double d2)
{
    return renderTileEntitySignParts(tileentitysign, d, d1, d2, false, true);
}

void TileEntitySignRenderer::renderTileEntityAt(TileEntity *tileentity, double d, double d1, double d2, float f)
{
    renderTileEntitySignAt(dynamic_cast<TileEntitySign *>(tileentity), d, d1, d2, f);
}
