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
};
