#include "RenderItemFrame.h"
#include "EntityItemFrame.h"
#include "Direction.h"
#include "Block.h"
#include "Item.h"
#include "ItemStack.h"
#include "EntityItem.h"
#include "RenderItem.h"
#include "RenderManager.h"
#include "RenderEngine.h"
#include "ItemRenderer.h"
#include "MapItemRenderer.h"
#include "ItemMap.h"
#include "MapData.h"
#include "Tessellator.h"
#include "MathHelper.h"
#include "platform/RenderAPI.h"

RenderItemFrame::RenderItemFrame()
    : renderBlocksInstance()
{
}

static void renderBox(RenderBlocks &rb, Block *block, double minX, double minY, double minZ, double maxX, double maxY, double maxZ, int texture)
{
    block->setBlockBounds(minX, minY, minZ, maxX, maxY, maxZ);
    rb.overrideBlockTexture = texture;
    renderTranslate(-0.5f, -0.5f, -0.5f);
    Tessellator *tessellator = &Tessellator::instance;
    tessellator->startDrawingQuads();
    tessellator->setNormal(0.0f, -1.0f, 0.0f);
    rb.renderBottomFace(block, 0.0, 0.0, 0.0, texture);
    tessellator->setNormal(0.0f, 1.0f, 0.0f);
    rb.renderTopFace(block, 0.0, 0.0, 0.0, texture);
    tessellator->setNormal(0.0f, 0.0f, -1.0f);
    rb.renderEastFace(block, 0.0, 0.0, 0.0, texture);
    tessellator->setNormal(0.0f, 0.0f, 1.0f);
    rb.renderWestFace(block, 0.0, 0.0, 0.0, texture);
    tessellator->setNormal(-1.0f, 0.0f, 0.0f);
    rb.renderNorthFace(block, 0.0, 0.0, 0.0, texture);
    tessellator->setNormal(1.0f, 0.0f, 0.0f);
    rb.renderSouthFace(block, 0.0, 0.0, 0.0, texture);
    tessellator->draw();
    renderTranslate(0.5f, 0.5f, 0.5f);
    rb.overrideBlockTexture = -1;
}

void RenderItemFrame::renderFrameItemAsBlock(EntityItemFrame *var1)
{
    renderPushMatrix();
    renderManager->renderEngine->bindTexture(renderManager->renderEngine->getTexture("/terrain.png"));
    renderRotate(var1->rotationYaw, 0.0f, 1.0f, 0.0f);
    Block *var2 = Block::planks;
    float var3 = 0.0625f;
    float var4 = 0.75f;
    float var5 = var4 / 2.0f;

    int birchTex = var2->getBlockTextureFromSideAndMetadata(2, 2);

    // Back plate
    renderPushMatrix();
    renderBox(renderBlocksInstance, var2, 0.0, (double)(0.5f - var5 + 0.0625f), (double)(0.5f - var5 + 0.0625f),
              (double)(var3 * 0.5f), (double)(0.5f + var5 - 0.0625f), (double)(0.5f + var5 - 0.0625f), birchTex);
    renderPopMatrix();

    // Top border
    renderPushMatrix();
    renderBox(renderBlocksInstance, var2, 0.0, (double)(0.5f - var5), (double)(0.5f - var5),
              (double)(var3 + 1.0e-4f), (double)(var3 + 0.5f - var5), (double)(0.5f + var5), birchTex);
    renderPopMatrix();

    // Bottom border
    renderPushMatrix();
    renderBox(renderBlocksInstance, var2, 0.0, (double)(0.5f + var5 - var3), (double)(0.5f - var5),
              (double)(var3 + 1.0e-4f), (double)(0.5f + var5), (double)(0.5f + var5), birchTex);
    renderPopMatrix();

    // Left border
    renderPushMatrix();
    renderBox(renderBlocksInstance, var2, 0.0, (double)(0.5f - var5), (double)(0.5f - var5),
              (double)var3, (double)(0.5f + var5), (double)(var3 + 0.5f - var5), birchTex);
    renderPopMatrix();

    // Right border
    renderPushMatrix();
    renderBox(renderBlocksInstance, var2, 0.0, (double)(0.5f - var5), (double)(0.5f + var5 - var3),
              (double)var3, (double)(0.5f + var5), (double)(0.5f + var5), birchTex);
    renderPopMatrix();

    var2->setBlockBounds(0.0, 0.0, 0.0, 1.0, 1.0, 1.0);
    renderPopMatrix();
}

void RenderItemFrame::func_82402_b(EntityItemFrame *var1)
{
    ItemStack *var2 = var1->getDisplayedItem();
    if (var2 != nullptr)
    {
        // The entity owns its stack (DataWatcher) and frees it when it goes out of
        // scope; Java shares the frame's stack, so give it a copy instead.
        EntityItem var3(var1->worldObj, 0.0, 0.0, 0.0, var2->copy());
        if (var3.getEntityItem() != nullptr)
        {
            var3.getEntityItem()->stackSize = 1;
        }
        var3.hoverStart = 0.0f;
        renderPushMatrix();
        renderTranslate(-0.453125f * (float)Direction::offsetX[var1->hangingDirection], -0.18f, -0.453125f * (float)Direction::offsetZ[var1->hangingDirection]);
        renderRotate(180.0f + var1->rotationYaw, 0.0f, 1.0f, 0.0f);
        renderRotate((float)(-90 * var1->getRotation()), 0.0f, 0.0f, 1.0f);
        switch (var1->getRotation())
        {
        case 1:
            renderTranslate(-0.16f, -0.16f, 0.0f);
            break;
        case 2:
            renderTranslate(0.0f, -0.32f, 0.0f);
            break;
        case 3:
            renderTranslate(0.16f, -0.16f, 0.0f);
            break;
        }

        if (var3.getEntityItem() != nullptr && Item::map != nullptr && var3.getEntityItem()->itemID == Item::map->shiftedIndex)
        {
            renderManager->renderEngine->bindTexture(renderManager->renderEngine->getTexture("/misc/mapbg.png"));
            Tessellator *var4 = &Tessellator::instance;
            renderRotate(180.0f, 0.0f, 1.0f, 0.0f);
            renderRotate(180.0f, 0.0f, 0.0f, 1.0f);
            renderScale(0.00390625f, 0.00390625f, 0.00390625f);
            renderTranslate(-65.0f, -107.0f, -3.0f);
            var4->startDrawingQuads();
            var4->setNormal(0.0f, 0.0f, -1.0f);
            char var5 = 7;
            var4->addVertexWithUV((double)(0 - var5), (double)(128 + var5), 0.0, 0.0, 1.0);
            var4->addVertexWithUV((double)(128 + var5), (double)(128 + var5), 0.0, 1.0, 1.0);
            var4->addVertexWithUV((double)(128 + var5), (double)(0 - var5), 0.0, 1.0, 0.0);
            var4->addVertexWithUV((double)(0 - var5), (double)(0 - var5), 0.0, 0.0, 0.0);
            var4->draw();
            MapData *var6 = ((ItemMap*)Item::map)->getMapData(var3.getEntityItem(), var1->worldObj);
            renderTranslate(0.0f, 0.0f, -1.0f);
            if (var6 != nullptr && renderManager->itemRenderer != nullptr && renderManager->itemRenderer->field_28131_f != nullptr)
            {
                renderManager->itemRenderer->field_28131_f->renderMap(nullptr, renderManager->renderEngine, var6);
            }
        }
        else
        {
            RenderItem::renderInFrame = true;
            RenderManager::instance->renderEntityWithPosYaw(&var3, 0.0, 0.0, 0.0, 0.0f, 0.0f);
            RenderItem::renderInFrame = false;
        }

        renderPopMatrix();
    }
}

void RenderItemFrame::func_82404_a(EntityItemFrame *var1, double var2, double var4, double var6, float var8, float var9)
{
    renderPushMatrix();
    float var10 = (float)(var1->posX - var2) - 0.5f;
    float var11 = (float)(var1->posY - var4) - 0.5f;
    float var12 = (float)(var1->posZ - var6) - 0.5f;
    int var13 = var1->xPosition + Direction::offsetX[var1->hangingDirection];
    int var14 = var1->yPosition;
    int var15 = var1->zPosition + Direction::offsetZ[var1->hangingDirection];
    renderTranslate((float)var13 - var10, (float)var14 - var11, (float)var15 - var12);
    renderFrameItemAsBlock(var1);
    func_82402_b(var1);
    renderPopMatrix();
}

void RenderItemFrame::doRender(Entity *entity, double x, double y, double z, float yaw, float partialTicks)
{
    func_82404_a((EntityItemFrame*)entity, x, y, z, yaw, partialTicks);
}
