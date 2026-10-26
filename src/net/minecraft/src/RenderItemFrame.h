#pragma once
#include "RenderPainting.h"
class EntityItemFrame;
class RenderItemFrame final : public RenderPainting {
public:
    RenderItemFrame() : RenderPainting() {}
    void doRender(Entity *entity,double x,double y,double z,float yaw,float partial) override;
    void renderItemFrame(EntityItemFrame *frame,double x,double y,double z,float yaw,float partial);
    void renderDisplayedItem(EntityItemFrame *frame);
    void orientItem(int rotation);
    void bindFrameTexture();
    void drawFrameBack();
    void drawFrameFront();
    void drawFrameEdges();
    void restoreRenderState();
    void drawItemShadow(float scale);
    void drawItemHighlight(int rotation);
    void setItemBrightness(float brightness);
    float itemScale(const ItemStack *stack) const;
    bool hasDisplayedItem(const EntityItemFrame *frame) const;
    void translateToFrameSurface(const EntityItemFrame *frame);
    void rotateForFacing(int direction);
    void finishItemTransform();
    void renderItemFace(float size);
    void renderItemBack(float size);
    void renderItemSides(float size);
    void renderRotationMarker(int rotation);
    void pushItemLighting();
    void popItemLighting();
    void applyDepthOffset(float offset);
    void resetDepthOffset();
};
